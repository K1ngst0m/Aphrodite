#include "sceneWidgets.h"
#include <iomanip>
#include <sstream>

namespace aph
{

CameraControlWidget::CameraControlWidget(UI* pUI)
    : Widget(pUI)
{
    setupWidgets();
}

CameraControlWidget::~CameraControlWidget()
{
}

void CameraControlWidget::draw()
{
    if (!m_enabled)
    {
        return;
    }

    for (auto widget : m_widgets)
    {
        if (widget && widget->isEnabled())
        {
            widget->draw();
        }
    }
}

WidgetType CameraControlWidget::getType() const
{
    return WidgetType::CameraControl;
}

void CameraControlWidget::setCamera(Camera* pCamera)
{
    APH_ASSERT(pCamera);
    m_pCamera = pCamera;

    {
        // Initialize from camera's current settings
        m_isPerspective = (m_pCamera->getType() == CameraType::Perspective);
        // Update matrices
        updateCamera();
    }
}

void CameraControlWidget::setCameraType(CameraType type)
{
    bool isPerspective = (type == CameraType::Perspective);

    if (m_isPerspective != isPerspective)
    {
        m_isPerspective = isPerspective;

        if (m_pCamera)
        {
            // Instead of replacing the Camera object with assignment,
            // preserve the existing Camera object and just update relevant properties
            // This avoids corruption of memory managed by the object pool

            // Save existing camera properties we want to preserve
            Vec3 position = m_cameraPosition;
            Vec3 target   = m_cameraTarget;
            Vec3 up       = m_cameraUp;

            // Update the camera's projection based on the new type
            if (isPerspective)
            {
                m_pCamera->setProjection(PerspectiveInfo{
                    .aspect = m_aspectRatio,
                    .fov    = m_cameraFov,
                    .znear  = m_cameraNear,
                    .zfar   = m_cameraFar,
                });
            }
            else
            {
                m_pCamera->setProjection(OrthographicInfo{
                    .left   = m_orthoLeft,
                    .right  = m_orthoRight,
                    .bottom = m_orthoBottom,
                    .top    = m_orthoTop,
                    .znear  = m_cameraNear,
                    .zfar   = m_cameraFar,
                });
            }

            // Restore the camera's orientation
            m_pCamera->setLookAt(position, target, up);

            // Update UI elements
            updateCamera();
        }
    }
}

Camera* CameraControlWidget::getCamera() const
{
    return m_pCamera;
}

void CameraControlWidget::setPerspective(bool perspective)
{
    if (m_isPerspective != perspective)
    {
        setCameraType(perspective ? CameraType::Perspective : CameraType::Orthographic);
    }
}

bool CameraControlWidget::isPerspective() const
{
    return m_isPerspective;
}

void CameraControlWidget::setCameraPosition(const Vec3& position)
{
    m_cameraPosition = position;
    if (m_autoUpdate && m_pCamera)
    {
        updateCamera();
    }
}

const Vec3& CameraControlWidget::getCameraPosition() const
{
    return m_cameraPosition;
}

void CameraControlWidget::setCameraTarget(const Vec3& target)
{
    m_cameraTarget = target;
    if (m_autoUpdate && m_pCamera)
    {
        updateCamera();
    }
}

const Vec3& CameraControlWidget::getCameraTarget() const
{
    return m_cameraTarget;
}

void CameraControlWidget::setCameraUp(const Vec3& up)
{
    m_cameraUp = Normalize(up); // Ensure it's normalized
    if (m_autoUpdate && m_pCamera)
    {
        updateCamera();
    }
}

const Vec3& CameraControlWidget::getCameraUp() const
{
    return m_cameraUp;
}

void CameraControlWidget::setFOV(float fov)
{
    m_cameraFov = fov;
    if (m_autoUpdate && m_pCamera && m_isPerspective)
    {
        updateCamera();
    }
}

float CameraControlWidget::getFOV() const
{
    return m_cameraFov;
}

void CameraControlWidget::setOrthographicExtents(float left, float right, float bottom, float top)
{
    m_orthoLeft   = left;
    m_orthoRight  = right;
    m_orthoBottom = bottom;
    m_orthoTop    = top;

    if (m_autoUpdate && m_pCamera && !m_isPerspective)
    {
        updateCamera();
    }
}

void CameraControlWidget::getOrthographicExtents(float& left, float& right, float& bottom, float& top) const
{
    left   = m_orthoLeft;
    right  = m_orthoRight;
    bottom = m_orthoBottom;
    top    = m_orthoTop;
}

void CameraControlWidget::setNearClip(float nearClip)
{
    m_cameraNear = nearClip;
    if (m_autoUpdate && m_pCamera)
    {
        updateCamera();
    }
}

float CameraControlWidget::getNearClip() const
{
    return m_cameraNear;
}

void CameraControlWidget::setFarClip(float farClip)
{
    m_cameraFar = farClip;
    if (m_autoUpdate && m_pCamera)
    {
        updateCamera();
    }
}

float CameraControlWidget::getFarClip() const
{
    return m_cameraFar;
}

void CameraControlWidget::resetToDefaults()
{
    m_cameraPosition = { 0.0f, 0.0f, 3.0f };
    m_cameraTarget   = { 0.0f, 0.0f, 0.0f };
    m_cameraUp       = { 0.0f, 1.0f, 0.0f };
    m_cameraFov      = 60.0f;
    m_cameraNear     = 0.1f;
    m_cameraFar      = 100.0f;

    m_orthoLeft   = -5.0f;
    m_orthoRight  = 5.0f;
    m_orthoBottom = -5.0f;
    m_orthoTop    = 5.0f;

    if (m_pCamera)
    {
        setCameraType(CameraType::Perspective);
    }
}

void CameraControlWidget::updateCamera()
{
    if (!m_pCamera)
    {
        return;
    }

    // Update camera look-at
    m_pCamera->setLookAt(m_cameraPosition, m_cameraTarget, m_cameraUp);

    // Update projection based on camera type
    if (m_isPerspective)
    {
        m_pCamera->setProjection(PerspectiveInfo{
            .aspect = m_aspectRatio,
            .fov    = m_cameraFov,
            .znear  = m_cameraNear,
            .zfar   = m_cameraFar,
        });
    }
    else
    {
        m_pCamera->setProjection(OrthographicInfo{
            .left   = m_orthoLeft,
            .right  = m_orthoRight,
            .bottom = m_orthoBottom,
            .top    = m_orthoTop,
            .znear  = m_cameraNear,
            .zfar   = m_cameraFar,
        });
    }

    // Update view matrix row displays
    if (m_viewMatrixRow1)
    {
        auto viewMatrix = m_pCamera->getView();
        m_viewMatrixRow1->setText(formatMatrix4Row(viewMatrix, 0));
        m_viewMatrixRow2->setText(formatMatrix4Row(viewMatrix, 1));
        m_viewMatrixRow3->setText(formatMatrix4Row(viewMatrix, 2));
        m_viewMatrixRow4->setText(formatMatrix4Row(viewMatrix, 3));
    }

    // Update projection matrix row displays
    if (m_projMatrixRow1)
    {
        auto projMatrix = m_pCamera->getProjection();
        m_projMatrixRow1->setText(formatMatrix4Row(projMatrix, 0));
        m_projMatrixRow2->setText(formatMatrix4Row(projMatrix, 1));
        m_projMatrixRow3->setText(formatMatrix4Row(projMatrix, 2));
        m_projMatrixRow4->setText(formatMatrix4Row(projMatrix, 3));
    }

    // Update the camera type and properties display
    if (m_cameraTypeInfo)
    {
        m_cameraTypeInfo->setText(m_isPerspective ? "Perspective" : "Orthographic");
    }

    // Update distance display
    if (m_distanceInfo)
    {
        float distance = Distance(m_cameraPosition, m_cameraTarget);
        std::stringstream distSS;
        distSS << std::fixed << std::setprecision(2) << distance;
        m_distanceInfo->setText(distSS.str());
    }

    // Update direction display
    if (m_directionInfo)
    {
        Vec3 direction = Normalize(m_cameraTarget - m_cameraPosition);
        std::stringstream dirSS;
        dirSS << std::fixed << std::setprecision(2) << "(" << direction.x << ", " << direction.y << ", " << direction.z
              << ")";
        m_directionInfo->setText(dirSS.str());
    }
}

std::string CameraControlWidget::formatMatrix4Row(const Mat4& matrix, int row, int precision)
{
    // GLM matrices are column-major, so we need to use matrix[column][row]
    std::stringstream ss;
    ss << std::fixed << std::setprecision(precision);
    ss << "[ ";
    // Access each column for the given row
    for (int col = 0; col < 4; ++col)
    {
        ss << matrix[col][row];
        if (col < 3)
            ss << ", ";
    }
    ss << " ]";
    return ss.str();
}

void CameraControlWidget::setupWidgets()
{
    if (!m_pUI)
    {
        return;
    }

    // ===== Camera Type Selection =====
    {
        // Add Camera Type radio buttons
        auto cameraTypeLabel = m_pUI->createWidget<Label>();
        cameraTypeLabel->setLabel("Camera Type");
        m_widgets.push_back(cameraTypeLabel);

        auto perspectiveRadio = m_pUI->createWidget<RadioButton>();
        perspectiveRadio->setLabel("Perspective");
        perspectiveRadio->setValue(m_isPerspective);
        perspectiveRadio->setCallback(
            [this](bool value)
            {
                if (value)
                {
                    setCameraType(CameraType::Perspective);
                }
            });
        m_widgets.push_back(perspectiveRadio);

        auto orthoRadio = m_pUI->createWidget<RadioButton>();
        orthoRadio->setLabel("Orthographic");
        orthoRadio->setValue(!m_isPerspective);
        orthoRadio->setCallback(
            [this](bool value)
            {
                if (value)
                {
                    setCameraType(CameraType::Orthographic);
                }
            });
        m_widgets.push_back(orthoRadio);
    }

    // ===== Separator =====
    {
        auto separator1 = m_pUI->createWidget<Separator>();
        m_widgets.push_back(separator1);
    }

    // ===== Camera Position Controls =====
    {
        // Add Camera Position controls
        auto posLabel = m_pUI->createWidget<Label>();
        posLabel->setLabel("Camera Position");
        m_widgets.push_back(posLabel);

        auto posSlider = m_pUI->createWidget<SliderFloat3>();
        posSlider->setLabel("Position");
        posSlider->setValue(m_cameraPosition);
        posSlider->setRange(-10.0f, 10.0f);
        posSlider->setCallback(
            [this](const Vec3& position)
            {
                m_cameraPosition = position;
                updateCamera();
            });
        m_widgets.push_back(posSlider);
    }

    // ===== Camera Target Controls =====
    {
        // Add Camera Target controls
        auto targetLabel = m_pUI->createWidget<Label>();
        targetLabel->setLabel("Look-At Target");
        m_widgets.push_back(targetLabel);

        auto targetSlider = m_pUI->createWidget<SliderFloat3>();
        targetSlider->setLabel("Target");
        targetSlider->setValue(m_cameraTarget);
        targetSlider->setRange(-10.0f, 10.0f);
        targetSlider->setCallback(
            [this](const Vec3& target)
            {
                m_cameraTarget = target;
                updateCamera();
            });
        m_widgets.push_back(targetSlider);
    }

    // ===== Camera Up Vector Controls =====
    {
        // Add Camera Up vector controls
        auto upLabel = m_pUI->createWidget<Label>();
        upLabel->setLabel("Up Vector");
        m_widgets.push_back(upLabel);

        auto upSlider = m_pUI->createWidget<SliderFloat3>();
        upSlider->setLabel("Up");
        upSlider->setValue(m_cameraUp);
        upSlider->setRange(-1.0f, 1.0f);
        upSlider->setCallback(
            [this](const Vec3& up)
            {
                m_cameraUp = Normalize(up); // Normalize to ensure unit vector
                updateCamera();
            });
        m_widgets.push_back(upSlider);
    }

    // ===== Separator =====
    {
        auto separator2 = m_pUI->createWidget<Separator>();
        m_widgets.push_back(separator2);
    }

    // ===== Perspective Camera Parameters =====
    {
        // Perspective camera parameters
        auto perspectiveHeader = m_pUI->createWidget<CollapsingHeader>();
        perspectiveHeader->setLabel("Perspective Parameters");
        m_widgets.push_back(perspectiveHeader);

        auto fovSlider = m_pUI->createWidget<SliderFloat>();
        fovSlider->setLabel("Field of View");
        fovSlider->setValue(m_cameraFov);
        fovSlider->setRange(1.0f, 179.0f);
        fovSlider->setCallback(
            [this](float fov)
            {
                m_cameraFov = fov;
                if (m_isPerspective)
                {
                    updateCamera();
                }
            });
        perspectiveHeader->addWidget(fovSlider);
    }

    // ===== Orthographic Camera Parameters =====
    {
        // Orthographic camera parameters
        auto orthoHeader = m_pUI->createWidget<CollapsingHeader>();
        orthoHeader->setLabel("Orthographic Parameters");
        m_widgets.push_back(orthoHeader);

        auto leftSlider = m_pUI->createWidget<SliderFloat>();
        leftSlider->setLabel("Left");
        leftSlider->setValue(m_orthoLeft);
        leftSlider->setRange(-20.0f, 0.0f);
        leftSlider->setCallback(
            [this](float value)
            {
                m_orthoLeft = value;
                if (!m_isPerspective)
                {
                    updateCamera();
                }
            });
        orthoHeader->addWidget(leftSlider);

        auto rightSlider = m_pUI->createWidget<SliderFloat>();
        rightSlider->setLabel("Right");
        rightSlider->setValue(m_orthoRight);
        rightSlider->setRange(0.0f, 20.0f);
        rightSlider->setCallback(
            [this](float value)
            {
                m_orthoRight = value;
                if (!m_isPerspective)
                {
                    updateCamera();
                }
            });
        orthoHeader->addWidget(rightSlider);

        auto bottomSlider = m_pUI->createWidget<SliderFloat>();
        bottomSlider->setLabel("Bottom");
        bottomSlider->setValue(m_orthoBottom);
        bottomSlider->setRange(-20.0f, 0.0f);
        bottomSlider->setCallback(
            [this](float value)
            {
                m_orthoBottom = value;
                if (!m_isPerspective)
                {
                    updateCamera();
                }
            });
        orthoHeader->addWidget(bottomSlider);

        auto topSlider = m_pUI->createWidget<SliderFloat>();
        topSlider->setLabel("Top");
        topSlider->setValue(m_orthoTop);
        topSlider->setRange(0.0f, 20.0f);
        topSlider->setCallback(
            [this](float value)
            {
                m_orthoTop = value;
                if (!m_isPerspective)
                {
                    updateCamera();
                }
            });
        orthoHeader->addWidget(topSlider);
    }

    // ===== Common Camera Parameters =====
    {
        // Common parameters for both camera types
        auto commonHeader = m_pUI->createWidget<CollapsingHeader>();
        commonHeader->setLabel("Common Parameters");
        m_widgets.push_back(commonHeader);

        auto nearSlider = m_pUI->createWidget<SliderFloat>();
        nearSlider->setLabel("Near Clip");
        nearSlider->setValue(m_cameraNear);
        nearSlider->setRange(0.01f, 10.0f);
        nearSlider->setFormat("%.2f");
        nearSlider->setCallback(
            [this](float value)
            {
                m_cameraNear = value;
                updateCamera();
            });
        commonHeader->addWidget(nearSlider);

        auto farSlider = m_pUI->createWidget<SliderFloat>();
        farSlider->setLabel("Far Clip");
        farSlider->setValue(m_cameraFar);
        farSlider->setRange(10.0f, 1000.0f);
        farSlider->setFormat("%.1f");
        farSlider->setCallback(
            [this](float value)
            {
                m_cameraFar = value;
                updateCamera();
            });
        commonHeader->addWidget(farSlider);
    }

    // ===== Separator =====
    {
        auto separator3 = m_pUI->createWidget<Separator>();
        m_widgets.push_back(separator3);
    }

    // ===== Reset Button =====
    {
        // Add reset button
        auto resetButton = m_pUI->createWidget<Button>();
        resetButton->setLabel("Reset Camera");
        resetButton->setCallback(
            [this]()
            {
                resetToDefaults();
            });
        m_widgets.push_back(resetButton);
    }

    // ===== Separator =====
    {
        auto separator4 = m_pUI->createWidget<Separator>();
        m_widgets.push_back(separator4);
    }

    // ===== Matrix Information =====
    {
        // Display current matrix info
        auto matrixHeader = m_pUI->createWidget<CollapsingHeader>();
        matrixHeader->setLabel("Matrix Information");
        m_widgets.push_back(matrixHeader);

        // View matrix display
        auto viewMatrixLabel = m_pUI->createWidget<Label>();
        viewMatrixLabel->setLabel("View Matrix");
        matrixHeader->addWidget(viewMatrixLabel);

        // Create view matrix row displays
        m_viewMatrixRow1 = m_pUI->createWidget<DynamicText>();
        m_viewMatrixRow1->setText("[ 0, 0, 0, 0 ]");
        m_viewMatrixRow2 = m_pUI->createWidget<DynamicText>();
        m_viewMatrixRow2->setText("[ 0, 0, 0, 0 ]");
        m_viewMatrixRow3 = m_pUI->createWidget<DynamicText>();
        m_viewMatrixRow3->setText("[ 0, 0, 0, 0 ]");
        m_viewMatrixRow4 = m_pUI->createWidget<DynamicText>();
        m_viewMatrixRow4->setText("[ 0, 0, 0, 0 ]");

        matrixHeader->addWidget(m_viewMatrixRow1);
        matrixHeader->addWidget(m_viewMatrixRow2);
        matrixHeader->addWidget(m_viewMatrixRow3);
        matrixHeader->addWidget(m_viewMatrixRow4);

        // Add a small spacing
        auto spacer = m_pUI->createWidget<HorizontalSpace>();
        spacer->setWidth(5.0f);
        matrixHeader->addWidget(spacer);

        // Projection matrix display
        auto projMatrixLabel = m_pUI->createWidget<Label>();
        projMatrixLabel->setLabel("Projection Matrix");
        matrixHeader->addWidget(projMatrixLabel);

        // Create projection matrix row displays
        m_projMatrixRow1 = m_pUI->createWidget<DynamicText>();
        m_projMatrixRow1->setText("[ 0, 0, 0, 0 ]");
        m_projMatrixRow2 = m_pUI->createWidget<DynamicText>();
        m_projMatrixRow2->setText("[ 0, 0, 0, 0 ]");
        m_projMatrixRow3 = m_pUI->createWidget<DynamicText>();
        m_projMatrixRow3->setText("[ 0, 0, 0, 0 ]");
        m_projMatrixRow4 = m_pUI->createWidget<DynamicText>();
        m_projMatrixRow4->setText("[ 0, 0, 0, 0 ]");

        matrixHeader->addWidget(m_projMatrixRow1);
        matrixHeader->addWidget(m_projMatrixRow2);
        matrixHeader->addWidget(m_projMatrixRow3);
        matrixHeader->addWidget(m_projMatrixRow4);
    }

    // ===== Camera Properties =====
    {
        // Add a section to show current camera properties as text
        auto propertiesHeader = m_pUI->createWidget<CollapsingHeader>();
        propertiesHeader->setLabel("Camera Properties");
        m_widgets.push_back(propertiesHeader);

        // Camera type info
        m_cameraTypeInfo = m_pUI->createWidget<DynamicText>();
        m_cameraTypeInfo->setLabel("Type");
        m_cameraTypeInfo->setText(m_isPerspective ? "Perspective" : "Orthographic");
        propertiesHeader->addWidget(m_cameraTypeInfo);

        // Distance to target
        m_distanceInfo = m_pUI->createWidget<DynamicText>();
        m_distanceInfo->setLabel("Distance to Target");
        m_distanceInfo->setText("0.0");
        propertiesHeader->addWidget(m_distanceInfo);

        // Camera orientation
        m_directionInfo = m_pUI->createWidget<DynamicText>();
        m_directionInfo->setLabel("Look Direction");
        m_directionInfo->setText("(0.0, 0.0, 0.0)");
        propertiesHeader->addWidget(m_directionInfo);
    }
}

} // namespace aph
