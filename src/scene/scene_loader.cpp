#include <scene/scene_loader.h>
#include <scene/scene.h>
#include <app/application.h>
#include <script/script_registry.h>
#include <utils/filesystem.h>
#include <utils/bullet_glm_helpers.h>
#include <physic/physics_loader.h>
#include <scene/component_loader.h>
#include <app/config_loader.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

std::vector<entt::entity> SceneLoader::Load(const std::string &filePath, Scene &scene, ResourceManager &res, PhysicsWorld &phys, SoundManager &sound, Application *app)
{
    std::string fullPath = FileSystem::getPath(filePath);
    std::ifstream file(fullPath);

    if (!file.is_open())
    {
        std::cerr << "[SceneLoader] Could not open scene file: " << fullPath << std::endl;
        return {};
    }

    std::vector<entt::entity> loadedEntities;
    std::map<entt::entity, std::vector<std::string>> deferredChildren;
    entt::entity currentEntity = entt::null;

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
            res.LoadShader(name, vs, fs);
        }
        else if (command == "LOAD_MODEL")
        {
            std::string name, path;
            ss >> name >> path;
            res.LoadModel(name, path, false);
        }
        else if (command == "LOAD_STATIC_MODEL")
        {
            std::string name, path;
            ss >> name >> path;
            res.LoadModel(name, path, true);
        }
        else if (command == "LOAD_ANIMATION")
        {
            std::string name, modelName, path;
            ss >> name >> modelName >> path;
            res.LoadAnimation(name, path, modelName);
        }
        else if (command == "LOAD_FONT")
        {
            std::string name, path;
            int size;
            ss >> name >> path >> size;
            res.LoadFont(name, path, size);
        }
        else if (command == "LOAD_SOUND")
        {
            std::string name, path;
            ss >> name >> path;
            res.LoadSound(name, path, sound.GetEngine());
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

            res.LoadSkybox(name, faces);
        }
        else if (command == "CONFIG")
        {
            ConfigLoader::LoadConfig(ss, app);
        }
        else if (command == "NEW_ENTITY")
        {
            currentEntity = scene.createEntity();
            loadedEntities.push_back(currentEntity);
            std::string entityName = "unnamed";
            std::string entityTag = "default";
            if (ss >> entityName)
            {
                ss >> entityTag;
            }
            scene.registry.emplace<InfoComponent>(currentEntity, entityName, entityTag);
        }
        else if (command == "TRANSFORM")
        {
            float x, y, z, rx, ry, rz, sx, sy, sz;
            ss >> x >> y >> z >> rx >> ry >> rz >> sx >> sy >> sz;
            auto &t = scene.registry.emplace<TransformComponent>(currentEntity);
            t.position = glm::vec3(x, y, z);
            t.rotation = glm::quat(glm::vec3(rx, ry, rz));
            t.scale = glm::vec3(sx, sy, sz);
        }
        else if (command == "VIDEO_MAP")
        {
            // VIDEO_MAP scaleX scaleY offsetX offsetY
            float sx = 1.0f, sy = 1.0f, ox = 0.0f, oy = 0.0f;
            ss >> sx >> sy >> ox >> oy;
            
            auto &mat = scene.registry.get_or_emplace<MaterialComponent>(currentEntity);
            mat.uvScale = glm::vec2(sx, sy);
            mat.uvOffset = glm::vec2(ox, oy);
        }
        else if (command == "RENDERER")
        {
            ComponentLoader::LoadRenderer(scene, currentEntity, ss, res);
        }
        else if (command == "ANIMATOR")
        {
            ComponentLoader::LoadAnimator(scene, currentEntity, ss, res);
        }
        else if (command == "CAMERA")
        {
            ComponentLoader::LoadCamera(scene, currentEntity, ss);
        }
        else if (command == "RIGIDBODY")
        {
            PhysicsLoader::LoadRigidBody(scene, currentEntity, ss, phys, file);
        }
        else if (command == "LIGHT_DIR")
        {
            float dx, dy, dz, r, g, b, i;
            ss >> dx >> dy >> dz >> r >> g >> b >> i;
            auto &l = scene.registry.emplace<DirectionalLightComponent>(currentEntity);
            l.direction = glm::vec3(dx, dy, dz);
            l.color = glm::vec3(r, g, b);
            l.intensity = i;

            float ambientStr = 0.2f;
            float diffuseStr = 0.8f;

            if (ss >> ambientStr)
            {
                if (ss >> diffuseStr)
                {
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
            auto &l = scene.registry.emplace<PointLightComponent>(currentEntity);
            l.color = glm::vec3(r, g, b);
            l.intensity = i;
            l.radius = rad;

            float c, lin, quad;
            if (ss >> c >> lin >> quad)
            {
                l.constant = c;
                l.linear = lin;
                l.quadratic = quad;
            }

            float ambStr = 0.1f;
            float diffStr = 1.0f;
            if (ss >> ambStr)
            {
                if (ss >> diffStr)
                {
                }
            }
            l.ambient = l.color * ambStr;
            l.diffuse = l.color * diffStr;
            l.specular = glm::vec3(1.0f);
        }
        else if (command == "LIGHT_SPOT")
        {
            float r, g, b, i, cut, outer;
            ss >> r >> g >> b >> i >> cut >> outer;
            auto &l = scene.registry.emplace<SpotLightComponent>(currentEntity);
            l.color = glm::vec3(r, g, b);
            l.intensity = i;
            l.cutOff = glm::cos(glm::radians(cut));
            l.outerCutOff = glm::cos(glm::radians(outer));

            float c, lin, quad;
            if (ss >> c >> lin >> quad)
            {
                l.constant = c;
                l.linear = lin;
                l.quadratic = quad;
            }

            float ambStr = 0.1f;
            float diffStr = 1.0f;
            if (ss >> ambStr)
            {
                if (ss >> diffStr)
                {
                }
            }
            l.ambient = l.color * ambStr;
            l.diffuse = l.color * diffStr;
            l.specular = glm::vec3(1.0f);
        }
        else if (command == "UI_TRANSFORM")
        {
            float x, y, w, h;
            int z;
            ss >> x >> y >> w >> h >> z;
            scene.registry.emplace<UITransformComponent>(currentEntity, glm::vec2(x, y), glm::vec2(w, h), z);
        }
        else if (command == "UI_RENDERER")
        {
            float r, g, b, a;
            std::string shaderName;
            ss >> r >> g >> b >> a >> shaderName;
            auto &ui = scene.registry.emplace<UIRendererComponent>(currentEntity);
            ui.color = glm::vec4(r, g, b, a);
            ui.shader = res.GetShader(shaderName);

            if (!res.GetUIModel("default_rect"))
                res.CreateUIModel("default_rect", UIType::Color);
            ui.model = res.GetUIModel("default_rect");
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

            auto &txt = scene.registry.emplace<UITextComponent>(currentEntity);
            txt.text = textContent;
            txt.font = res.GetFont(fontName);
            txt.color = glm::vec3(r, g, b);
            txt.scale = scale;

            if (!res.GetUIModel("default_text_rect"))
                res.CreateUIModel("default_text_rect", UIType::Text);
            txt.model = res.GetUIModel("default_text_rect");
            txt.shader = res.GetShader("textShader");
        }
        else if (command == "UI_ANIMATION")
        {
            float hr, hg, hb, ha;
            ss >> hr >> hg >> hb >> ha;
            auto &anim = scene.registry.emplace<UIAnimationComponent>(currentEntity);
            anim.hoverColor = glm::vec4(hr, hg, hb, ha);

            auto &ui = scene.registry.get<UIRendererComponent>(currentEntity);
            anim.normalColor = ui.color;
        }
        else if (command == "SKYBOX_RENDERER")
        {
            std::string skyboxName, shaderName;
            ss >> skyboxName >> shaderName;

            auto &comp = scene.registry.emplace<SkyboxRenderComponent>(currentEntity);
            comp.skybox = res.GetSkybox(skyboxName);
            comp.shader = res.GetShader(shaderName);
        }
        else if (command == "SCRIPT")
        {
            std::string className;
            ss >> className;

            auto &scriptComp = scene.registry.emplace<ScriptComponent>(currentEntity);

            Scriptable *scriptInstance = ScriptRegistry::Instance().Create(className);

            if (scriptInstance)
            {
                scriptComp.instance = scriptInstance;
                scriptComp.InstantiateScript = [className]()
                { return ScriptRegistry::Instance().Create(className); };
                scriptComp.DestroyScript = [](ScriptComponent *nsc)
                { delete nsc->instance; nsc->instance = nullptr; };
                scriptComp.instance->Init(currentEntity, &scene, app);
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

            scene.registry.emplace<AudioSourceComponent>(currentEntity, audio);
        }
        else if (command == "VIDEO_PLAYER")
        {
            std::string path;
            int loop = 0, playOnAwake = 1;
            float speed = 1.0f;
            ss >> path;

            if (!ss.eof()) ss >> loop;
            if (!ss.eof()) ss >> speed;
            if (!ss.eof()) ss >> playOnAwake;

            VideoPlayerComponent video;
            video.filePath = FileSystem::getPath(path);
            video.isLooping = (loop != 0);
            video.speed = speed;
            video.playOnAwake = (playOnAwake != 0);

            scene.registry.emplace<VideoPlayerComponent>(currentEntity, video);
        }
        else if (command == "LOAD_PARTICLE")
        {
            std::string name, path;
            ss >> name >> path;
            res.LoadTexture(name, path);
        }
        else if (command == "MATERIAL")
        {
            std::string typeStr;
            ss >> typeStr;

            MaterialComponent mat;

            if (typeStr == "PBR")
            {

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

            scene.registry.emplace<MaterialComponent>(currentEntity, mat);
        }
        else if (command == "PARTICLE_EMITTER")
        {
            std::string texName;
            int maxParticles;
            float life;
            ss >> texName >> maxParticles >> life;

            auto &emitterComp = scene.registry.emplace<ParticleEmitterComponent>(currentEntity);
            emitterComp.emitter.Init(maxParticles);
            emitterComp.emitter.LifeTime = life;
            emitterComp.emitter.StartLife = life;

            emitterComp.emitter.texture = res.GetTexture(texName);

            if (!emitterComp.emitter.texture)
            {
                std::cerr << "[SceneManager] Particle Texture not found: " << texName << std::endl;
            }
        }
        else if (command == "PARENT")
        {
            std::string parentName;
            ss >> parentName;

            entt::entity parentEntity = entt::null;
            auto view = scene.registry.view<InfoComponent>();
            for (auto entity : view)
            {
                const auto &info = view.get<InfoComponent>(entity);
                if (info.name == parentName)
                {
                    parentEntity = entity;
                    break;
                }
            }

            if (parentEntity != entt::null)
            {
                 if (scene.registry.all_of<TransformComponent>(currentEntity) && 
                     scene.registry.all_of<TransformComponent>(parentEntity))
                 {
                     auto& transform = scene.registry.get<TransformComponent>(currentEntity);
                     transform.SetParent(currentEntity, parentEntity, scene.registry, true);
                 }
            }
            else
            {
                std::cerr << "[SceneManager] Parent not found: " << parentName << std::endl;
            }
        }
        else if (command == "CHILDREN")
        {
            std::string childName;
            while (ss >> childName)
            {
                 deferredChildren[currentEntity].push_back(childName);
            }
        }
    }

    if (!deferredChildren.empty())
    {
        auto view = scene.registry.view<InfoComponent>();
        for (const auto& [parentEntity, childNames] : deferredChildren)
        {
            for (const auto& childName : childNames)
            {
                entt::entity childEntity = entt::null;
                for (auto entity : view)
                {
                    if (view.get<InfoComponent>(entity).name == childName)
                    {
                        childEntity = entity;
                        break;
                    } 
                }

                if (childEntity != entt::null)
                {
                    if (scene.registry.all_of<TransformComponent>(childEntity) && 
                        scene.registry.all_of<TransformComponent>(parentEntity))
                    {
                        auto& transform = scene.registry.get<TransformComponent>(childEntity);
                        transform.SetParent(childEntity, parentEntity, scene.registry, true); 
                    }
                }
                else
                {
                   std::cerr << "[SceneLoader] Child not found: " << childName << " for Parent Entity ID: " << (uint32_t)parentEntity << std::endl;
                }
            }
        }
    }

    auto rbView = scene.registry.view<RigidBodyComponent, TransformComponent>();
    for (auto entity : rbView)
    {
        auto &rb = rbView.get<RigidBodyComponent>(entity);
        auto &transform = rbView.get<TransformComponent>(entity);

        if (rb.body)
        {
            // Sync RigidBody transform from Entity transform immediately on load
            glm::mat4 worldMatrix = transform.GetWorldModelMatrix(scene.registry);
            glm::vec3 position = glm::vec3(worldMatrix[3]);
            glm::quat rotation = glm::quat_cast(worldMatrix);

            btTransform tr;
            tr.setIdentity();
            tr.setOrigin(BulletGLMHelpers::convert(position));
            tr.setRotation(BulletGLMHelpers::convert(rotation));

            rb.body->setWorldTransform(tr);
            if(rb.body->getMotionState())
            {
                rb.body->getMotionState()->setWorldTransform(tr);
            }
            
            rb.body->setLinearVelocity(btVector3(0,0,0));
            rb.body->setAngularVelocity(btVector3(0,0,0));
            rb.body->activate();

            // Handle Physics Attachment (Joints)
            if (rb.isAttachedToParent && scene.registry.valid(transform.parent))
            {
                if (scene.registry.all_of<RigidBodyComponent>(transform.parent))
                {
                    auto& parentRb = scene.registry.get<RigidBodyComponent>(transform.parent);
                    if (parentRb.body)
                    {
                        btTransform frameInA, frameInB; // A = Parent, B = Child
                        
                        btTransform parentWorldTrans = parentRb.body->getWorldTransform();
                        btTransform childWorldTrans = rb.body->getWorldTransform();
                        
                        frameInA = parentWorldTrans.inverse() * childWorldTrans;
                        frameInB.setIdentity();

                        btFixedConstraint* fixedConstraint = new btFixedConstraint(
                            *parentRb.body,
                            *rb.body,
                            frameInA,
                            frameInB
                        );
                        
                        phys.AddConstraint(fixedConstraint);
                        rb.constraint = fixedConstraint;
                    }
                }
            }
        }
    }

    if (scene.GetActiveCamera() == entt::null)
    {
        std::cout << "[SceneLoader] WARNING: No Active Camera found in scene! Creating Default Spectator Camera." << std::endl;
        
        entt::entity camEntity = scene.createEntity();
        loadedEntities.push_back(camEntity);
        
        scene.registry.emplace<InfoComponent>(camEntity, "Default Spectator Camera", "Default");
        
        auto& trans = scene.registry.emplace<TransformComponent>(camEntity);
        trans.position = glm::vec3(0.0f, 2.0f, 10.0f);
        
        auto& cam = scene.registry.emplace<CameraComponent>(camEntity);
        cam.isPrimary = true;
        cam.fov = 45.0f;
        cam.nearPlane = 0.1f;
        cam.farPlane = 1000.0f;
        cam.worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
        
        std::string scriptName = "DefaultCameraController";
        Scriptable* scriptInstance = ScriptRegistry::Instance().Create(scriptName);
        
        if (scriptInstance)
        {
            auto& scriptComp = scene.registry.emplace<ScriptComponent>(camEntity);
            scriptComp.instance = scriptInstance;
            scriptComp.InstantiateScript = [scriptName](){ return ScriptRegistry::Instance().Create(scriptName); };
            scriptComp.DestroyScript = [](ScriptComponent* nsc){ delete nsc->instance; nsc->instance = nullptr; };
            
            scriptComp.instance->Init(camEntity, &scene, app);
            scriptComp.instance->OnCreate();
            std::cout << "[SceneLoader] Attached 'DefaultCameraController' (Engine Fallback) to default camera." << std::endl;
        }
        else
        {
            std::cout << "[SceneLoader] 'DefaultCameraController' script not found! Make sure it is compiled." << std::endl;
        }
    }

    return loadedEntities;
}
