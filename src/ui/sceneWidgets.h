#pragma once

#include "imgui.h"
#include "ui.h"
#include "widget.h"
#include "widgets.h"
#include "scene/camera.h"

namespace aph
{
//
// Scene-related Widgets
//

// Camera control widget that allows manipulation of camera parameters
class CameraControlWidget : public Widget
{
public:
    explicit CameraControlWidget(UI* pUI);
    ~CameraControlWidget();

    // Camera access
    void setCamera(Camera* pCamera);
    Camera* getCamera() const;

    // Camera type
    void setPerspective(bool perspective);
    bool isPerspective() const;
    void setCameraType(CameraType type);

    // Camera position controls
    void setCameraPosition(const Vec3& position);
    const Vec3& getCameraPosition() const;

    void setCameraTarget(const Vec3& target);
    const Vec3& getCameraTarget() const;

    void setCameraUp(const Vec3& up);
    const Vec3& getCameraUp() const;

    // Perspective parameters
    void setFOV(float fov);
    float getFOV() const;

    // Orthographic parameters
    void setOrthographicExtents(float left, float right, float bottom, float top);
    void getOrthographicExtents(float& left, float& right, float& bottom, float& top) const;

    // Common parameters
    void setNearClip(float nearClip);
    float getNearClip() const;

    void setFarClip(float farClip);
    float getFarClip() const;

    // Reset to default settings
    void resetToDefaults();

    // Update camera from internal parameters
    void updateCamera();

    // Drawing
    void draw() override;
    
    // Widget type
    WidgetType getType() const override;

private:
    // Helper to format matrix rows for display
    std::string formatMatrix4Row(const Mat4& matrix, int row, int precision = 2);

    // Create the widget UI components
    void setupWidgets();

private:
    // Camera reference
    Camera* m_pCamera = nullptr;

    // Camera parameters
    Vec3 m_cameraPosition = {0.0f, 0.0f, 3.0f};
    Vec3 m_cameraTarget = {0.0f, 0.0f, 0.0f};
    Vec3 m_cameraUp = {0.0f, 1.0f, 0.0f};
    float m_cameraFov = 60.0f;
    float m_cameraNear = 0.1f;
    float m_cameraFar = 100.0f;
    bool m_isPerspective = true;
    float m_aspectRatio = 16.0f / 9.0f;

    // Orthographic parameters
    float m_orthoLeft = -5.0f;
    float m_orthoRight = 5.0f;
    float m_orthoBottom = -5.0f;
    float m_orthoTop = 5.0f;

    // Internal widgets for the UI
    SmallVector<Widget*> m_widgets;

    // Matrix display widgets
    DynamicText* m_viewMatrixRow1 = nullptr;
    DynamicText* m_viewMatrixRow2 = nullptr;
    DynamicText* m_viewMatrixRow3 = nullptr;
    DynamicText* m_viewMatrixRow4 = nullptr;

    DynamicText* m_projMatrixRow1 = nullptr;
    DynamicText* m_projMatrixRow2 = nullptr;
    DynamicText* m_projMatrixRow3 = nullptr;
    DynamicText* m_projMatrixRow4 = nullptr;

    // Camera info display widgets
    DynamicText* m_cameraTypeInfo = nullptr;
    DynamicText* m_distanceInfo = nullptr;
    DynamicText* m_directionInfo = nullptr;

    // Flag to control auto-update behavior
    bool m_autoUpdate = true;
};

} // namespace aph
