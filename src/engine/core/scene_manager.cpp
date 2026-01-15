#include <engine/core/scene_manager.h>
#include <engine/core/script_registry.h>
#include <engine/utils/filesystem.h>
#include <engine/utils/filesystem.h>
#include <engine/utils/bullet_glm_helpers.h>
#include <engine/core/application.h>

SceneManager::SceneManager(Scene &scene, ResourceManager &res, PhysicsWorld &phys, SoundManager &sound, Application *app)
    : m_Scene(scene), m_Resources(res), m_Physics(phys), m_SoundManager(sound), m_App(app) {}

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
        else if (command == "LOAD_SKYBOX")
        {
            std::string name;
            ss >> name;

            std::vector<std::string> faces(6);
            for (int i = 0; i < 6; i++)
            {
                std::string path;
                ss >> path;
                faces[i] = FileSystem::getPath(path);
            }

            m_Resources.LoadSkybox(name, faces);
        }

        else if (command == "CONFIG")
        {
            std::string subCmd;
            ss >> subCmd;
            if (subCmd == "SHADOWS")
            {
                int enable = 0;
                ss >> enable;
                if (m_App)
                {
                    m_App->GetRenderSystem().SetEnableShadows(enable != 0);
                }
            }
            else if (subCmd == "CULL_FACE")
            {
                int enable = 0;
                std::string modeStr;
                ss >> enable;
                if (enable)
                {
                    ss >> modeStr;
                    int mode = GL_BACK;
                    if (modeStr == "FRONT") mode = GL_FRONT;
                    else if (modeStr == "FRONT_AND_BACK") mode = GL_FRONT_AND_BACK;
                    
                    if (m_App) m_App->GetRenderSystem().SetFaceCulling(true, mode);
                }
                else
                {
                    if (m_App) m_App->GetRenderSystem().SetFaceCulling(false);
                }
            }
            else if (subCmd == "DEPTH_TEST")
            {
                int enable = 0;
                std::string funcStr;
                ss >> enable;
                if (enable)
                {
                    ss >> funcStr;
                    int func = GL_LESS;
                    if (funcStr == "NEVER") func = GL_NEVER;
                    else if (funcStr == "LESS") func = GL_LESS;
                    else if (funcStr == "EQUAL") func = GL_EQUAL;
                    else if (funcStr == "LEQUAL") func = GL_LEQUAL;
                    else if (funcStr == "GREATER") func = GL_GREATER;
                    else if (funcStr == "NOTEQUAL") func = GL_NOTEQUAL;
                    else if (funcStr == "GEQUAL") func = GL_GEQUAL;
                    else if (funcStr == "ALWAYS") func = GL_ALWAYS;

                    if (m_App) m_App->GetRenderSystem().SetDepthTest(true, func);
                }
                else
                {
                    if (m_App) m_App->GetRenderSystem().SetDepthTest(false);
                }
            }
            else if (subCmd == "WINDOW")
            {
                int w, h;
                if (ss >> w >> h)
                {
                    std::string modeStr;
                    int monitorIdx = 0;
                    WindowMode mode = WindowMode::WINDOWED;

                    if (ss >> modeStr)
                    {
                        if (modeStr == "FULLSCREEN") mode = WindowMode::FULLSCREEN;
                        else if (modeStr == "BORDERLESS") mode = WindowMode::BORDERLESS;
                        else mode = WindowMode::WINDOWED;
                    }
                    
                    if (!ss.eof())
                        ss >> monitorIdx;
                    
                    int refreshRate = 0;
                    if (!ss.eof())
                        ss >> refreshRate;

                    if (m_App) m_App->SetWindowConfiguration(w, h, mode, monitorIdx, refreshRate);
                }
            }
            else if (subCmd == "VSYNC")
            {
                int enable = 0;
                if (ss >> enable)
                {
                    if (m_App) m_App->SetVsync(enable != 0);
                }
            }
            else if (subCmd == "FPS")
            {
                int fps = 0;
                if (ss >> fps)
                {
                    if (m_App) m_App->SetFrameRateLimit(fps);
                }
            }
        }

        else if (command == "NEW_ENTITY")
        {
            currentEntity = m_Scene.createEntity();
            sceneEntities.push_back(currentEntity);
            std::string entityName = "unnamed";
            std::string entityTag = "default";
            if (ss >> entityName)
            {
                ss >> entityTag;
            }
            m_Scene.registry.emplace<InfoComponent>(currentEntity, entityName, entityTag);
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
            float nearPlane = 0.1f;
            float farPlane = 1000.0f;

            ss >> isPrimary >> fov >> yaw >> pitch;

            if (!ss.eof())
                ss >> nearPlane;
            if (!ss.eof())
                ss >> farPlane;

            auto &c = m_Scene.registry.emplace<CameraComponent>(currentEntity);
            c.isPrimary = (bool)isPrimary;
            c.fov = fov;
            c.yaw = yaw;
            c.pitch = pitch;
            c.nearPlane = nearPlane;
            c.farPlane = farPlane;
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
                std::string bodyType = "UNKNOWN";

                if (ss >> bodyType)
                {
                    // Found explicit type
                }
                else
                {
                    // Deduce fram mass
                    if (mass > 0.0f)
                        bodyType = "DYNAMIC";
                    else
                        bodyType = "STATIC";
                }

                if (bodyType == "STATIC")
                    mass = 0.0f;
                if (bodyType == "KINEMATIC")
                    mass = 0.0f;

                btTransform transform;
                transform.setIdentity();
                transform.setOrigin(BulletGLMHelpers::convert(trans.position));
                transform.setRotation(BulletGLMHelpers::convert(trans.rotation));

                rb.body = m_Physics.CreateRigidBody(mass, transform, finalShape);

                if (rb.body)
                {
                    if (bodyType == "KINEMATIC")
                    {
                        rb.body->setCollisionFlags(rb.body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
                        rb.body->setActivationState(DISABLE_DEACTIVATION);
                    }

                    rb.body->setUserPointer((void *)(uintptr_t)currentEntity);

                    if (type == "CAPSULE" || type == "PLAYER")
                        rb.body->setAngularFactor(btVector3(0, 1, 0));
                }
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

            // Default coefficients
            float ambientStr = 0.2f;
            float diffuseStr = 0.8f;

            // Try parsing optional ambient/diffuse coefficients
            if (ss >> ambientStr)
            {
                if (ss >> diffuseStr)
                {
                    // Successfully read both
                }
            }

            l.ambient = l.color * ambientStr;
            l.diffuse = l.color * diffuseStr;
            l.specular = glm::vec3(0.5f);
        }
        else if (command == "LIGHT_POINT")
        {
            float r, g, b, i, rad;
            ss >> r >> g >> b >> i >> rad;
            auto &l = m_Scene.registry.emplace<PointLightComponent>(currentEntity);
            l.color = glm::vec3(r, g, b);
            l.intensity = i;
            l.radius = rad;

            // Optional: Parse attenuation if provided
            float c, lin, quad;
            if (ss >> c >> lin >> quad)
            {
                l.constant = c;
                l.linear = lin;
                l.quadratic = quad;
            }

            // Optional: Parse Ambient/Diffuse
            float ambStr = 0.1f; // Default point/spot ambient is usually low
            float diffStr = 1.0f;
            if (ss >> ambStr)
            {
                if (ss >> diffStr)
                {
                    // Read both
                }
            }
            l.ambient = l.color * ambStr;
            l.diffuse = l.color * diffStr;
            l.specular = glm::vec3(1.0f); // Specular white for point/spot often makes sense, or derive from color?
        }
        else if (command == "LIGHT_SPOT")
        {
            float r, g, b, i, cut, outer;
            ss >> r >> g >> b >> i >> cut >> outer;
            auto &l = m_Scene.registry.emplace<SpotLightComponent>(currentEntity);
            l.color = glm::vec3(r, g, b);
            l.intensity = i;
            l.cutOff = glm::cos(glm::radians(cut));
            l.outerCutOff = glm::cos(glm::radians(outer));

            // Optional: Parse attenuation if provided
            float c, lin, quad;
            if (ss >> c >> lin >> quad)
            {
                l.constant = c;
                l.linear = lin;
                l.quadratic = quad;
            }

            // Optional: Parse Ambient/Diffuse
            float ambStr = 0.1f;
            float diffStr = 1.0f;
            if (ss >> ambStr)
            {
                if (ss >> diffStr)
                {
                    // Read both
                }
            }
            l.ambient = l.color * ambStr;
            l.diffuse = l.color * diffStr;
            l.specular = glm::vec3(1.0f);
        }
        else if (command == "RENDERER" || command == "MODEL")
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
        else if (command == "SKYBOX_RENDERER")
        {
            std::string skyboxName, shaderName;
            ss >> skyboxName >> shaderName;

            auto &comp = m_Scene.registry.emplace<SkyboxRenderComponent>(currentEntity);
            comp.skybox = m_Resources.GetSkybox(skyboxName);
            comp.shader = m_Resources.GetShader(shaderName);
        }
        else if (command == "SCRIPT")
        {
            std::string className;
            ss >> className;

            auto &scriptComp = m_Scene.registry.emplace<ScriptComponent>(currentEntity);

            Scriptable *scriptInstance = ScriptRegistry::Instance().Create(className);

            if (scriptInstance)
            {
                scriptComp.instance = scriptInstance;
                scriptComp.InstantiateScript = [className]()
                { return ScriptRegistry::Instance().Create(className); };
                scriptComp.DestroyScript = [](ScriptComponent *nsc)
                { delete nsc->instance; nsc->instance = nullptr; };
                // Init script immediately
                scriptComp.instance->Init(currentEntity, &m_Scene, m_App);
                scriptComp.instance->OnCreate();
            }
        }
        else if (command == "AUDIO_SOURCE")
        {
            std::string path;
            float vol, minDur;
            int loop, is3d, awake;
            ss >> path >> vol >> loop >> is3d >> minDur >> awake;

            AudioSourceComponent audio;
            audio.filePath = path;
            audio.volume = vol;
            audio.loop = (loop != 0);
            audio.is3D = (is3d != 0);
            audio.minDistance = minDur;
            audio.playOnAwake = (awake != 0);

            m_Scene.registry.emplace<AudioSourceComponent>(currentEntity, audio);
        }
        else if (command == "LOAD_PARTICLE")
        {
            std::string name, path;
            ss >> name >> path;
            m_Resources.LoadTexture(name, path);
        }
        else if (command == "MATERIAL")
        {
            std::string typeStr;
            ss >> typeStr;

            MaterialComponent mat;

            if (typeStr == "PBR")
            {
                mat.type = MaterialType::PBR;
                ss >> mat.roughness >> mat.metallic >> mat.ao;

                float er = 0.0f, eg = 0.0f, eb = 0.0f;
                if (ss >> er >> eg >> eb)
                {
                    mat.emission = glm::vec3(er, eg, eb);
                }
            }
            else if (typeStr == "PHONG")
            {
                mat.type = MaterialType::PHONG;
                float r = 0.5f, g = 0.5f, b = 0.5f;
                ss >> mat.shininess >> r >> g >> b;
                mat.specular = glm::vec3(r, g, b);

                float er = 0.0f, eg = 0.0f, eb = 0.0f;
                if (ss >> er >> eg >> eb)
                {
                    mat.emission = glm::vec3(er, eg, eb);
                }

                float ar = 1.0f, ag = 1.0f, ab = 1.0f;
                if (ss >> ar >> ag >> ab)
                {
                    mat.ambient = glm::vec3(ar, ag, ab);
                }
            }
            else
            {
                try
                {
                    mat.type = MaterialType::PHONG;
                    mat.shininess = std::stof(typeStr);

                    float r = 0.5f, g = 0.5f, b = 0.5f;
                    if (ss >> r >> g >> b)
                    {
                        mat.specular = glm::vec3(r, g, b);
                    }
                }
                catch (...)
                {
                    std::cout << "Invalid MATERIAL format for entity " << (int)currentEntity << std::endl;
                }
            }

            m_Scene.registry.emplace<MaterialComponent>(currentEntity, mat);
        }
        else if (command == "PARTICLE_EMITTER")
        {
            std::string texName;
            int maxParticles;
            float life;
            ss >> texName >> maxParticles >> life;

            auto &emitterComp = m_Scene.registry.emplace<ParticleEmitterComponent>(currentEntity);
            emitterComp.emitter.Init(maxParticles);
            emitterComp.emitter.LifeTime = life;
            emitterComp.emitter.StartLife = life;

            emitterComp.emitter.texture = m_Resources.GetTexture(texName);

            if (!emitterComp.emitter.texture)
            {
                std::cerr << "[SceneManager] Particle Texture not found: " << texName << std::endl;
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
        m_Scene.destroyEntity(entity, this);
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

void SceneManager::QueueLoadScene(const std::string &path)
{
    m_pendingPath = path;
    m_isPending = true;
    std::cout << "[SceneManager] Queued load scene: " << path << std::endl;
}

void SceneManager::UpdatePendingScene()
{
    if (m_isPending)
    {
        ClearAllScenes();
        LoadScene(m_pendingPath);
        m_isPending = false;
        m_pendingPath = "";
    }
}