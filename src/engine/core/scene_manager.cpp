#include <engine/core/scene_manager.h>
#include <engine/utils/filesystem.h>
#include <engine/utils/bullet_glm_helpers.h>

SceneManager::SceneManager(Scene &scene, ResourceManager &res, PhysicsWorld &phys, SoundManager &sound)
    : m_Scene(scene), m_Resources(res), m_Physics(phys), m_SoundManager(sound) {}

void SceneManager::LoadScene(const std::string &filePath)
{
    if (m_LoadedScenes.find(filePath) != m_LoadedScenes.end())
    {
        std::cout << "[SceneManager] Scene already loaded: " << filePath << std::endl;
        return;
    }

    std::string fullPath = FileSystem::getPath(filePath);
    std::ifstream file(fullPath);

    if (!file.is_open())
    {
        std::cerr << "[SceneManager] Could not open scene file: " << fullPath << std::endl;
        return;
    }

    std::vector<entt::entity> &sceneEntities = m_LoadedScenes[filePath];

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        std::stringstream ss(line);
        std::string command;
        ss >> command;

        if (command == "LOAD_SHADER")
        {
            std::string name, vs, fs;
            ss >> name >> vs >> fs;
            m_Resources.LoadShader(name, vs, fs);
        }
        if (command == "LOAD_MODEL")
        {
            std::string name, path;
            ss >> name >> path;
            m_Resources.LoadModel(name, path, false);
        }
        else if (command == "LOAD_STATIC_MODEL")
        {
            std::string name, path;
            ss >> name >> path;
            m_Resources.LoadModel(name, path, true);
        }
        else if (command == "LOAD_ANIMATION")
        {
            std::string name, modelName, path;
            ss >> name >> modelName >> path;
            m_Resources.LoadAnimation(name, path, modelName);
        }
        else if (command == "LOAD_FONT")
        {
            std::string name, path;
            int size;
            ss >> name >> path >> size;
            m_Resources.LoadFont(name, path, size);
        }
        else if (command == "LOAD_SOUND")
        {
            std::string name, path;
            ss >> name >> path;
            m_Resources.LoadSound(name, path, m_SoundManager.GetEngine());
        }

        else if (command == "NEW_ENTITY")
        {
            currentEntity = m_Scene.createEntity();
            sceneEntities.push_back(currentEntity);
            std::string entityName;
            if (ss >> entityName)
            {
                m_Scene.registry.emplace<InfoComponent>(currentEntity, entityName);
            }
            else
            {
                m_Scene.registry.emplace<InfoComponent>(currentEntity, "Unnamed");
            }
        }

        else if (command == "TRANSFORM")
        {
            float x, y, z, rx, ry, rz, sx, sy, sz;
            ss >> x >> y >> z >> rx >> ry >> rz >> sx >> sy >> sz;
            auto &t = m_Scene.registry.emplace<TransformComponent>(currentEntity);
            t.position = glm::vec3(x, y, z);
            t.rotation = glm::quat(glm::vec3(rx, ry, rz));
            t.scale = glm::vec3(sx, sy, sz);
        }
        else if (command == "RENDERER")
        {
            std::string modelName, shaderName;
            ss >> modelName >> shaderName;
            auto &r = m_Scene.registry.emplace<MeshRendererComponent>(currentEntity);
            r.model = m_Resources.GetModel(modelName);
            r.shader = m_Resources.GetShader(shaderName);
        }
        else if (command == "ANIMATOR")
        {
            std::string animName;
            ss >> animName;

            float speed = 1.0f;
            float startTime = 0.0f;
            float rate = 30.0f;

            if (ss >> speed)
            {
                if (ss >> startTime)
                {
                    ss >> rate;
                }
            }

            auto &a = m_Scene.registry.emplace<AnimationComponent>(currentEntity);

            a.animator = new Animator(m_Resources.GetAnimation(animName));
            a.animator->SetSpeed(speed);
            a.animator->SetTime(startTime);
            a.animator->SetUpdateRate(rate);
        }
        else if (command == "CAMERA")
        {
            int isPrimary;
            float fov, yaw, pitch;
            ss >> isPrimary >> fov >> yaw >> pitch;
            auto &c = m_Scene.registry.emplace<CameraComponent>(currentEntity);
            c.isPrimary = (bool)isPrimary;
            c.fov = fov;
            c.yaw = yaw;
            c.pitch = pitch;
        }
        else if (command == "RIGIDBODY")
        {
            std::string type;
            float mass;
            ss >> type >> mass;

            auto &trans = m_Scene.registry.get<TransformComponent>(currentEntity);
            auto &rb = m_Scene.registry.emplace<RigidBodyComponent>(currentEntity);

            btCollisionShape *finalShape = nullptr;
            if (type == "COMPOUND")
            {
                btCompoundShape *compound = new btCompoundShape();
                std::string subLine;

                while (std::getline(file, subLine))
                {
                    std::stringstream subSS(subLine);
                    std::string subCmd;
                    subSS >> subCmd;

                    if (subCmd == "END_RIGIDBODY")
                        break;

                    if (subCmd == "SHAPE")
                    {
                        std::string shapeType;
                        float lx, ly, lz, lrx, lry, lrz;
                        subSS >> shapeType >> lx >> ly >> lz >> lrx >> lry >> lrz;

                        btTransform localTrans;
                        localTrans.setIdentity();
                        localTrans.setOrigin(btVector3(lx, ly, lz));
                        btQuaternion localRot;
                        localRot.setEuler(glm::radians(lry), glm::radians(lrx), glm::radians(lrz));
                        localTrans.setRotation(localRot);

                        btCollisionShape *childShape = nullptr;

                        if (shapeType == "BOX")
                        {
                            float x, y, z;
                            subSS >> x >> y >> z;
                            childShape = new btBoxShape(btVector3(x, y, z));
                        }
                        else if (shapeType == "SPHERE")
                        {
                            float r;
                            subSS >> r;
                            childShape = new btSphereShape(r);
                        }
                        else if (shapeType == "CAPSULE")
                        {
                            float r, h;
                            subSS >> r >> h;
                            childShape = new btCapsuleShape(r, h);
                        }

                        if (childShape)
                        {
                            compound->addChildShape(localTrans, childShape);
                            m_Physics.RegisterShape(childShape);
                        }
                    }
                }
                finalShape = compound;
            }
            if (type == "CAPSULE")
            {
                float r, h;
                ss >> r >> h;
                finalShape = new btCapsuleShape(r, h);
            }
            else if (type == "BOX")
            {
                float x, y, z;
                ss >> x >> y >> z;
                finalShape = new btBoxShape(btVector3(x, y, z));
            }
            if (finalShape)
            {
                btTransform transform;
                transform.setIdentity();
                transform.setOrigin(BulletGLMHelpers::convert(trans.position));
                transform.setRotation(BulletGLMHelpers::convert(trans.rotation));

                rb.body = m_Physics.CreateRigidBody(mass, transform, finalShape);

                if (type == "CAPSULE" || type == "PLAYER")
                    if (rb.body)
                        rb.body->setAngularFactor(btVector3(0, 1, 0));
            }
        }
        else if (command == "LIGHT_DIR")
        {
            float dx, dy, dz, r, g, b, i;
            ss >> dx >> dy >> dz >> r >> g >> b >> i;
            auto &l = m_Scene.registry.emplace<DirectionalLightComponent>(currentEntity);
            l.direction = glm::vec3(dx, dy, dz);
            l.color = glm::vec3(r, g, b);
            l.intensity = i;
        }

        else if (command == "UI_TRANSFORM")
        {
            float x, y, w, h;
            int z;
            ss >> x >> y >> w >> h >> z;
            m_Scene.registry.emplace<UITransformComponent>(currentEntity, glm::vec2(x, y), glm::vec2(w, h), z);
        }
        else if (command == "UI_RENDERER")
        {
            float r, g, b, a;
            std::string shaderName;
            ss >> r >> g >> b >> a >> shaderName;
            auto &ui = m_Scene.registry.emplace<UIRendererComponent>(currentEntity);
            ui.color = glm::vec4(r, g, b, a);
            ui.shader = m_Resources.GetShader(shaderName);

            // Mặc định tạo model Color cho UI (hoặc thêm lệnh LOAD_UI_TEXTURE sau)
            if (!m_Resources.GetUIModel("default_rect"))
                m_Resources.CreateUIModel("default_rect", UIType::Color);
            ui.model = m_Resources.GetUIModel("default_rect");
        }
        else if (command == "UI_TEXT")
        {
            std::string textContent;
            std::string fontName;
            float r, g, b, scale;

            ss >> std::ws;
            if (ss.peek() == '"')
            {
                char quote;
                ss >> quote;
                std::getline(ss, textContent, '"');
            }
            else
            {
                ss >> textContent;
            }

            ss >> fontName >> r >> g >> b >> scale;

            auto &txt = m_Scene.registry.emplace<UITextComponent>(currentEntity);
            txt.text = textContent;
            txt.font = m_Resources.GetFont(fontName);
            txt.color = glm::vec3(r, g, b);
            txt.scale = scale;

            if (!m_Resources.GetUIModel("default_text_rect"))
                m_Resources.CreateUIModel("default_text_rect", UIType::Text);
            txt.model = m_Resources.GetUIModel("default_text_rect");
            txt.shader = m_Resources.GetShader("textShader");
        }
        else if (command == "UI_ANIMATION")
        {
            float hr, hg, hb, ha;
            ss >> hr >> hg >> hb >> ha;
            auto &anim = m_Scene.registry.emplace<UIAnimationComponent>(currentEntity);
            anim.hoverColor = glm::vec4(hr, hg, hb, ha);

            auto &ui = m_Scene.registry.get<UIRendererComponent>(currentEntity);
            anim.normalColor = ui.color;
        }
        else if (command == "SCRIPT")
        {
            std::string scriptName;
            ss >> scriptName;

            if (m_ScriptRegistry.find(scriptName) != m_ScriptRegistry.end())
            {
                m_ScriptRegistry[scriptName](m_Scene, currentEntity);
            }
            else
            {
                std::cout << "[SceneManager] Warning: Script '" << scriptName << "' not found in registry!" << std::endl;
            }
        }
    }
}

void SceneManager::UnloadScene(const std::string &filePath)
{
    auto it = m_LoadedScenes.find(filePath);
    if (it == m_LoadedScenes.end())
    {
        std::cout << "[SceneManager] Scene not found or not loaded: " << filePath << std::endl;
        return;
    }

    for (auto entity : it->second)
    {
        DestroyEntity(entity);
    }

    m_LoadedScenes.erase(it);
}

void SceneManager::ClearAllScenes()
{
    m_Scene.registry.clear();

    m_Physics.Clear();

    m_LoadedScenes.clear();
    currentEntity = entt::null;
}

void SceneManager::DestroyEntity(entt::entity entity)
{
    if (m_Scene.registry.all_of<RigidBodyComponent>(entity))
    {
        auto &rb = m_Scene.registry.get<RigidBodyComponent>(entity);
        if (rb.body)
        {
            m_Physics.GetWorld()->removeRigidBody(rb.body);

            if (rb.body->getMotionState())
                delete rb.body->getMotionState();

            delete rb.body;
            rb.body = nullptr;
        }
    }

    if (m_Scene.registry.all_of<AnimationComponent>(entity))
    {
        auto &anim = m_Scene.registry.get<AnimationComponent>(entity);
        if (anim.animator)
        {
            delete anim.animator;
            anim.animator = nullptr;
        }
    }

    if (m_Scene.registry.valid(entity))
    {
        m_Scene.registry.destroy(entity);
    }
}