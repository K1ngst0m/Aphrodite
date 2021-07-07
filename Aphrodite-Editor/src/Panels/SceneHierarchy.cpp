//
// Created by npchitman on 6/28/21.
//

#include "SceneHierarchy.h"

#include <Aphrodite/Utils/PlatformUtils.h>
#include <imgui.h>
#include <imgui_internal.h>

#include <cstring>
#include <glm/gtc/type_ptr.hpp>

#include "Aphrodite/Scene/Components.h"

namespace Aph::Editor {
    SceneHierarchy::SceneHierarchy(const Ref<Scene>& context) {
        SetContext(context);
    }

    void SceneHierarchy::SetContext(const Ref<Scene>& context) {
        m_Context = context;
        m_SelectionContext = {};
    }

    void SceneHierarchy::OnImGuiRender() {
        ImGui::Begin(Style::Title::SceneHierarchy.data());

        m_Context->m_Registry.each([&](auto entityID) {
            Entity entity{entityID, m_Context.get()};
            DrawEntityNode(entity);
        });

        if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
            m_SelectionContext = {};

        // Right click
        if (ImGui::BeginPopupContextWindow(nullptr, 1, false)) {
            if (ImGui::MenuItem("Create Empty"))
                m_SelectionContext = m_Context->CreateEntity("Empty");
            else if (ImGui::MenuItem("Create Camera")) {
                m_SelectionContext = m_Context->CreateEntity("Camera");
                m_SelectionContext.AddComponent<CameraComponent>();
                ImGui::CloseCurrentPopup();
            } else if (ImGui::MenuItem("Create Sprite")) {
                m_SelectionContext = m_Context->CreateEntity("Sprite");
                m_SelectionContext.AddComponent<SpriteRendererComponent>();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::End();

        ImGui::Begin(Style::Title::Properties.data());
        if (m_SelectionContext) {
            DrawComponents(m_SelectionContext);
        }
        ImGui::End();
    }

    void SceneHierarchy::SetSelectedEntity(Entity entity) {
        m_SelectionContext = entity;
    }

    void SceneHierarchy::DrawEntityNode(Entity entity) {
        auto& tagComponent = entity.GetComponent<TagComponent>();
        auto& tag = tagComponent.Tag;

        ImGuiTreeNodeFlags flags = ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
        flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
        bool opened = ImGui::TreeNodeEx((void*) (uint64_t) (uint32_t) entity, flags, "%s", tag.c_str());
        if (ImGui::IsItemClicked()) {
            m_SelectionContext = entity;
        }

        bool entityDeleted = false;
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Rename"))
                tagComponent.renaming = true;
            if (ImGui::MenuItem("Delete Entity"))
                entityDeleted = true;

            ImGui::EndPopup();
        }

        if (tagComponent.renaming) {
            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            std::strncpy(buffer, tag.c_str(), sizeof(buffer));
            if (ImGui::InputText("##Tag", buffer, sizeof(buffer))) {
                tag = std::string(buffer);
            }

            if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered()) {
                tagComponent.renaming = false;
            }
        }

        if (opened) {
            flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
            opened = ImGui::TreeNodeEx((void*) 9817239, flags, "%s", tag.c_str());
            if (opened)
                ImGui::TreePop();
            ImGui::TreePop();
        }

        if (entityDeleted) {
            m_Context->DestroyEntity(entity);
            if (m_SelectionContext == entity)
                m_SelectionContext = {};
        }
    }

    static void SetLabel(const char* label) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        const ImVec2 lineStart = ImGui::GetCursorScreenPos();
        const ImGuiStyle& style = ImGui::GetStyle();
        float fullWidth = ImGui::GetContentRegionAvail().x;
        float itemWidth = fullWidth * 0.6f;
        ImVec2 textSize = ImGui::CalcTextSize(label);
        ImRect textRect;
        textRect.Min = ImGui::GetCursorScreenPos();
        textRect.Max = textRect.Min;
        textRect.Max.x += fullWidth - itemWidth;
        textRect.Max.y += textSize.y;

        ImGui::SetCursorScreenPos(textRect.Min);

        ImGui::AlignTextToFramePadding();
        textRect.Min.y += window->DC.CurrLineTextBaseOffset;
        textRect.Max.y += window->DC.CurrLineTextBaseOffset;

        ImGui::ItemSize(textRect);
        if (ImGui::ItemAdd(textRect, window->GetID(label))) {
            ImGui::RenderTextEllipsis(ImGui::GetWindowDrawList(), textRect.Min, textRect.Max, textRect.Max.x,
                                      textRect.Max.x, label, nullptr, &textSize);

            if (textRect.GetWidth() < textSize.x && ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", label);
        }
        ImVec2 v(0, textSize.y + window->DC.CurrLineTextBaseOffset);
        ImGui::SetCursorScreenPos(ImVec2(textRect.Max.x - v.x, textRect.Max.y - v.y));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(itemWidth);
    }

    static void DrawFloatControl(const std::string& label, float* value, float min = 0, float max = 0, float columnWidth = 100.0f) {
        ImGui::PushID(label.c_str());
        SetLabel(label.c_str());
        ImGui::DragFloat("##value", value, 0.1f, min, max);
        ImGui::PopID();
    }

    static void DrawVec2Control(const std::string& label, glm::vec2& values, float resetValue = 0.0f, const char* format = "%.2f", float columnWidth = 200.0f) {
        ImGuiIO& io = ImGui::GetIO();
        auto boldFont = io.Fonts->Fonts[1];

        ImGui::PushID(label.c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text("%s", label.c_str());
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(2, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

        float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

        ImGui::PushStyleColor(ImGuiCol_Button, Style::Color::Vec3ButtonStyle.at("Default").X);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Style::Color::Vec3ButtonStyle.at("Hovered").X);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, Style::Color::Vec3ButtonStyle.at("Active").X);
        ImGui::PushFont(boldFont);
        if (ImGui::Button("X", buttonSize))
            values.x = resetValue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##X", &values.x, 0.1f, 0, 0, format);
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, Style::Color::Vec3ButtonStyle.at("Default").Y);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Style::Color::Vec3ButtonStyle.at("Hovered").Y);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, Style::Color::Vec3ButtonStyle.at("Active").Y);
        ImGui::PushFont(boldFont);
        if (ImGui::Button("Y", buttonSize))
            values.y = resetValue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Y", &values.y, 0.1f, 0, 0, format);
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();

        ImGui::Columns(1);

        ImGui::PopID();
    }

    static void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f) {
        ImGuiIO& io = ImGui::GetIO();
        auto boldFont = io.Fonts->Fonts[0];

        ImGui::PushID(label.c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text("%s", label.c_str());
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

        float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

        ImGui::PushStyleColor(ImGuiCol_Button, Style::Color::Vec3ButtonStyle.at("Default").X);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Style::Color::Vec3ButtonStyle.at("Hovered").X);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, Style::Color::Vec3ButtonStyle.at("Active").X);
        ImGui::PushFont(boldFont);
        if (ImGui::Button("X", buttonSize))
            values.x = resetValue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, Style::Color::Vec3ButtonStyle.at("Default").Y);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Style::Color::Vec3ButtonStyle.at("Hovered").Y);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, Style::Color::Vec3ButtonStyle.at("Active").Y);
        ImGui::PushFont(boldFont);
        if (ImGui::Button("Y", buttonSize))
            values.y = resetValue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, Style::Color::Vec3ButtonStyle.at("Default").Z);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Style::Color::Vec3ButtonStyle.at("Hovered").Z);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, Style::Color::Vec3ButtonStyle.at("Active").Z);
        ImGui::PushFont(boldFont);
        if (ImGui::Button("Z", buttonSize))
            values.z = resetValue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();

        ImGui::Columns(1);

        ImGui::PopID();
    }

    template<typename T, typename UIFunction>
    static void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction) {
        const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
        if (entity.HasComponent<T>()) {
            auto& component = entity.GetComponent<T>();
            ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{4, 4});
            float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
            ImGui::Separator();
            bool open = ImGui::TreeNodeEx((void*) typeid(T).hash_code(), treeNodeFlags, "%s", name.c_str());
            ImGui::PopStyleVar();
            ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
            if (ImGui::Button("+", ImVec2{lineHeight, lineHeight})) {
                ImGui::OpenPopup("ComponentSettings");
            }

            bool removeComponent = false;
            if (ImGui::BeginPopup("ComponentSettings")) {
                if (ImGui::MenuItem("Reset component")){
                    // TODO reset
                }

                if (ImGui::MenuItem("Remove component"))
                    removeComponent = true;

                ImGui::EndPopup();
            }

            if (open) {
                uiFunction(component);
                ImGui::TreePop();
            }

            if (removeComponent)
                entity.RemoveComponent<T>();
        }
    }


    void SceneHierarchy::DrawComponents(Entity entity) {
        if (entity.HasComponent<TagComponent>()) {
            auto& tag = entity.GetComponent<TagComponent>().Tag;

            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            std::strncpy(buffer, tag.c_str(), sizeof(buffer));
            if (ImGui::InputText("##Tag", buffer, sizeof(buffer))) {
                tag = std::string(buffer);
            }
        }

        ImGui::SameLine();
        ImGui::PushItemWidth(-1);

        if (ImGui::Button("Add Component"))
            ImGui::OpenPopup("AddComponent");

        if (ImGui::BeginPopup("AddComponent")) {
            if (ImGui::MenuItem("Transform")) {
                if (!m_SelectionContext.HasComponent<TransformComponent>())
                    m_SelectionContext.AddComponent<TransformComponent>();
                else
                    APH_CORE_WARN("This entity already has the Transform Component!");
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::MenuItem("Camera")) {
                if (!m_SelectionContext.HasComponent<CameraComponent>())
                    m_SelectionContext.AddComponent<CameraComponent>();
                else
                    APH_CORE_WARN("This entity already has the Camera Component!");
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::MenuItem("Sprite Renderer")) {
                if (!m_SelectionContext.HasComponent<SpriteRendererComponent>())
                    m_SelectionContext.AddComponent<SpriteRendererComponent>();
                else
                    APH_CORE_WARN("This entity already has the Sprite Renderer Component!");
                ImGui::CloseCurrentPopup();
            }


            if (ImGui::MenuItem("Rigidbody 2D")) {
                if (!entity.HasComponent<Rigidbody2DComponent>())
                    entity.AddComponent<Rigidbody2DComponent>();
                else
                    APH_CORE_WARN("This entity already has the Rigidbody 2D Component!");
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::MenuItem("Box Collider 2D")) {
                if (!entity.HasComponent<BoxCollider2DComponent>())
                    entity.AddComponent<BoxCollider2DComponent>();
                else
                    APH_CORE_WARN("This entity already has the Box Collider 2D Component!");
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::MenuItem("Skylight"))
            {
                if (!entity.HasComponent<SkylightComponent>())
                    entity.AddComponent<SkylightComponent>();
                else
                    APH_CORE_WARN("This entity already has the Skylight Component!");
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::PopItemWidth();

        DrawComponent<TransformComponent>("Transform", entity, [](TransformComponent& component) {
            DrawVec3Control("Translation", component.Translation);
            glm::vec3 rotation = glm::degrees(component.Rotation);
            DrawVec3Control("Rotation", rotation);
            component.Rotation = glm::radians(rotation);
            DrawVec3Control("Scale", component.Scale, 1.0f);
        });

        DrawComponent<CameraComponent>("Camera", entity, [](CameraComponent& component) {
            auto& camera = component.Camera;

            ImGui::Checkbox("Primary", &component.Primary);

            const char* projectionTypeStrings[] = {"Perspective", "Orthographic"};
            const char* currentProjectionTypeString = projectionTypeStrings[(int) camera.GetProjectionType()];

            if (ImGui::BeginCombo("Projection", currentProjectionTypeString)) {
                for (int i = 0; i < 2; i++) {
                    bool isSelected = currentProjectionTypeString == projectionTypeStrings[i];

                    if (ImGui::Selectable(projectionTypeStrings[i], isSelected)) {
                        currentProjectionTypeString = projectionTypeStrings[i];
                        camera.SetProjectionType((SceneCamera::ProjectionType) i);
                    }

                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                ImGui::EndCombo();
            }

            if (camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective) {
                float perspectiveVerticalFOV = glm::degrees(camera.GetPerspectiveVerticalFOV());
                if (ImGui::DragFloat("FOV", &perspectiveVerticalFOV)) {
                    camera.SetPerspectiveVerticalFOV(glm::radians(perspectiveVerticalFOV));
                }

                float perspectiveNear = camera.GetPerspectiveNearClip();
                if (ImGui::DragFloat("Near", &perspectiveNear)) {
                    camera.SetPerspectiveNearClip(perspectiveNear);
                }

                float perspectiveFar = camera.GetPerspectiveFarClip();
                if (ImGui::DragFloat("Far", &perspectiveFar)) {
                    camera.SetPerspectiveFarClip(perspectiveFar);
                }
            }

            if (camera.GetProjectionType() == SceneCamera::ProjectionType::Orthographic) {
                float orthoSize = camera.GetOrthographicSize();
                if (ImGui::DragFloat("Size", &orthoSize)) {
                    camera.SetOrthographicSize(orthoSize);
                }

                float orthoNear = camera.GetOrthographicNearClip();
                if (ImGui::DragFloat("Near", &orthoNear)) {
                    camera.SetOrthographicNearClip(orthoNear);
                }

                float orthoFar = camera.GetOrthographicFarClip();
                if (ImGui::DragFloat("Far", &orthoFar)) {
                    camera.SetOrthographicFarClip(orthoFar);
                }

                ImGui::Checkbox("Fixed Aspect Ratio", &component.FixedAspectRatio);
            }
        });

        DrawComponent<SpriteRendererComponent>("Sprite Renderer", entity, [](SpriteRendererComponent& component) {
            SetLabel("Color");
            ImGui::SameLine();
            ImGui::ColorEdit4("##Color", glm::value_ptr(component.Color));

          const uint32_t id = component.Texture == nullptr ? 0 : component.Texture->GetRendererID();

          SetLabel("Texture");
          const ImVec2 buttonSize = { 80, 80 };
          ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.25f, 0.25f, 0.25f, 1.0f });
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.35f, 0.35f, 0.35f, 1.0f });
          ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.25f, 0.25f, 0.25f, 1.0f });
          if(ImGui::ImageButton(reinterpret_cast<ImTextureID>(id), buttonSize, { 0, 1 }, { 1, 0}, 0))
          {
              std::string filepath = FileDialogs::OpenFile("Texture (*.png)\0*.png\0");
              if (!filepath.empty())
                  component.SetTexture(filepath);
          }
          ImGui::PopStyleColor(3);

          ImGui::SameLine();
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.2f, 0.2f, 1.0f });
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.3f, 0.3f, 1.0f });
          ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.2f, 0.2f, 1.0f });

          if(ImGui::Button("-", { buttonSize.x / 4, buttonSize.y } ))
              component.RemoveTexture();

          ImGui::PopStyleColor(3);
          ImGui::PopStyleVar();

          ImGui::Spacing();

          DrawFloatControl("Tiling Factor", &component.TilingFactor, 0, 0, 200); });

        DrawComponent<Rigidbody2DComponent>("Rigidbody 2D", entity, [](Rigidbody2DComponent& component) {
            {
                const char* items[] = {"Static", "Kinematic", "Dynamic"};
                const char* current_item = items[(int) component.Specification.Type];
                SetLabel("Body Type");
                ImGui::SameLine();
                if (ImGui::BeginCombo("##BodyType", current_item)) {
                    for (int n = 0; n < 3; n++) {
                        bool is_selected = (current_item == items[n]);
                        if (ImGui::Selectable(items[n], is_selected)) {
                            current_item = items[n];
                            component.Specification.Type = (Rigidbody2DType) n;
                        }

                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
            }

            if (component.Specification.Type == Rigidbody2DType::Dynamic) {
                DrawFloatControl("Linear Damping", &(component.Specification.LinearDamping), 0.0f, 1000000.0f, 200);
                DrawFloatControl("Angular Damping", &(component.Specification.AngularDamping), 0.0f, 1000000.0f, 200);
                DrawFloatControl("Gravity Scale", &(component.Specification.GravityScale), -1000000.0f, 1000000.0f, 200);
            }
            if (component.Specification.Type == Rigidbody2DType::Dynamic || component.Specification.Type == Rigidbody2DType::Kinematic) {
                {
                    const char* items[] = {"Discrete", "Continuous"};
                    const char* current_item = items[(int) component.Specification.CollisionDetection];
                    SetLabel("Collision Detection");
                    ImGui::SameLine();
                    if (ImGui::BeginCombo("##CollisionDetection", current_item)) {
                        for (int n = 0; n < 2; n++) {
                            bool is_selected = (current_item == items[n]);
                            if (ImGui::Selectable(items[n], is_selected)) {
                                current_item = items[n];
                                component.Specification.CollisionDetection = (Rigidbody2D::CollisionDetectionType) n;
                            }

                            if (is_selected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                }

                {
                    const char* items[] = {"NeverSleep", "StartAwake", "StartAsleep"};
                    const char* current_item = items[(int) component.Specification.SleepingMode];
                    SetLabel("Sleeping Mode");
                    ImGui::SameLine();
                    if (ImGui::BeginCombo("##SleepingMode", current_item)) {
                        for (int n = 0; n < 3; n++) {
                            bool is_selected = (current_item == items[n]);
                            if (ImGui::Selectable(items[n], is_selected)) {
                                current_item = items[n];
                                component.Specification.SleepingMode = (Rigidbody2D::SleepType) n;
                            }

                            if (is_selected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                }

                SetLabel("Freeze Rotation");
                ImGui::SameLine();
                ImGui::Checkbox("##FreezeRotationZ", &component.Specification.FreezeRotationZ);
                ImGui::SameLine();
                ImGui::Text("Z");
            }

            component.ValidateSpecification();
        });

        DrawComponent<BoxCollider2DComponent>("Box Collider 2D", entity, [](BoxCollider2DComponent& component) {
            SetLabel("Is Trigger");
            ImGui::SameLine();
            ImGui::Checkbox("##IsTrigger", &component.IsTrigger);

            SetLabel("Size");
            ImGui::SameLine();
            glm::vec2 size = component.Size;
            ImGui::DragFloat2("##Size", glm::value_ptr(size), 0.1f, 0, 0, "%.4f");
            if (size.x <= 0.1f)
                size.x = 0.1f;
            if (size.y <= 0.1f)
                size.y = 0.1f;
            component.Size = size;

            SetLabel("Offset");
            ImGui::SameLine();
            ImGui::DragFloat2("##Offset", glm::value_ptr(component.Offset), 0.1f, 0, 0, "%.4f");

            component.ValidateSpecification();
        });

        DrawComponent<SkylightComponent>("Skylight Component", entity, [](SkylightComponent& component)
        {
          const intptr_t id = component.Texture != nullptr ? component.Texture->GetHDRRendererID() : 0;

          SetLabel("Texture");
          const ImVec2 buttonSize = { 80, 80 };
          ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.25f, 0.25f, 0.25f, 1.0f });
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.35f, 0.35f, 0.35f, 1.0f });
          ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.25f, 0.25f, 0.25f, 1.0f });
          if (ImGui::ImageButton((ImTextureID)id, buttonSize, { 0, 1 }, { 1, 0 }, 0))
          {
              std::string filepath = FileDialogs::OpenFile("Texture (*.hdr)\0*.hdr\0");
              if (!filepath.empty())
                  component.SetTexture(filepath);
          }
          ImGui::PopStyleColor(3);

          ImGui::SameLine();
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.2f, 0.2f, 1.0f });
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.3f, 0.3f, 1.0f });
          ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.2f, 0.2f, 1.0f });
          if (ImGui::Button("x", { buttonSize.x / 4, buttonSize.y }))
              component.RemoveTexture();
          ImGui::PopStyleColor(3);
          ImGui::PopStyleVar();
        });
    }
}// namespace Aph
