#pragma once

#include "aph_core.hpp"

class RenderGraphVisualizer : public aph::App
{
public:
    RenderGraphVisualizer();

    void setExampleType(std::string_view value)
    {
        if (value == "simple")
        {
            m_exampleType = ExampleType::Simple;
        }
        else if (value == "complex")
        {
            m_exampleType = ExampleType::Complex;
        }
    }

private:
    void init() override;
    void load() override;
    void loop() override;
    void unload() override;
    void finish() override;

    // Create and configure the render graph pipeline
    void setupSimpleRenderGraph();
    void setupComplexRenderGraph();

    // Export the render graph to a file
    void exportRenderGraphToDot(const std::string& filename);

private:
    // The render graph we'll be visualizing (no GPU operations)
    std::unique_ptr<aph::RenderGraph> m_renderGraph;

    // Example type
    enum class ExampleType
    {
        Simple,
        Complex
    };

    ExampleType m_exampleType = ExampleType::Complex;
};
