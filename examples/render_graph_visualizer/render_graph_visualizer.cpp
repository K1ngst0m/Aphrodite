#include "render_graph_visualizer.h"

using namespace aph;

RenderGraphVisualizer::RenderGraphVisualizer()
    : aph::App("Render Graph Visualizer")
{
}

void RenderGraphVisualizer::init()
{
    APH_PROFILER_SCOPE();

    // Create a new render graph in dry run mode (no GPU operations)
    m_renderGraph = std::make_unique<RenderGraph>();

    // Enable debug output for detailed logging
    m_renderGraph->enableDebugOutput(true);

    // Setup the render graph example
    if (m_exampleType == ExampleType::Simple)
    {
        setupSimpleRenderGraph();
    }
    else
    {
        setupComplexRenderGraph();
    }
}

void RenderGraphVisualizer::setupSimpleRenderGraph()
{
    // Create a simple forward rendering pipeline
    auto* mainPass = m_renderGraph->createPass("MainPass", QueueType::Graphics);
    auto* postProcessPass = m_renderGraph->createPass("PostProcessPass", QueueType::Graphics);

    // Create image resources
    aph::vk::ImageCreateInfo colorInfo{
        .extent = { 1920, 1080, 1 },
        .usage = ImageUsage::ColorAttachment,
        .domain = MemoryDomain::Device,
        .imageType = ImageType::e2D,
        .format = Format::RGBA8_UNORM,
    };

    aph::vk::ImageCreateInfo depthInfo{
        .extent = { 1920, 1080, 1 },
        .usage = ImageUsage::DepthStencil,
        .domain = MemoryDomain::Device,
        .imageType = ImageType::e2D,
        .format = aph::Format::D32,
    };

    // Configure passes
    mainPass->setColorOut("SceneColor", { .createInfo = colorInfo });
    mainPass->setDepthStencilOut("SceneDepth", { .createInfo = depthInfo });

    postProcessPass->addTextureIn("SceneColor");
    postProcessPass->setColorOut("FinalColor", { .createInfo = colorInfo });

    // Set the back buffer
    m_renderGraph->setBackBuffer("FinalColor");

    // Register callbacks (these won't be executed in dry run mode)
    mainPass->recordExecute([](aph::vk::CommandBuffer*) {});
    postProcessPass->recordExecute([](aph::vk::CommandBuffer*) {});

    // Build the graph - performs dependency analysis and topological sorting
    m_renderGraph->build();

    // Execute the graph - in dry run mode, this will only output the execution order
    m_renderGraph->execute();

    // Export the visualization
    exportRenderGraphToDot("simple_render_graph.dot");
}

void RenderGraphVisualizer::setupComplexRenderGraph()
{
    // Create a more complex deferred rendering pipeline
    auto* geometryPass = m_renderGraph->createPass("GeometryPass", QueueType::Graphics);
    auto* lightingPass = m_renderGraph->createPass("LightingPass", QueueType::Graphics);
    auto* postProcessPass = m_renderGraph->createPass("PostProcessPass", QueueType::Graphics);
    auto* computePass = m_renderGraph->createPass("ComputePass", QueueType::Compute);
    auto* transferPass = m_renderGraph->createPass("TransferPass", QueueType::Transfer);

    // Create image resources
    aph::vk::ImageCreateInfo colorInfo{
        .extent = { 1920, 1080, 1 },
        .usage = ImageUsage::ColorAttachment,
        .domain = MemoryDomain::Device,
        .imageType = ImageType::e2D,
        .format = Format::RGBA8_UNORM,
    };

    aph::vk::ImageCreateInfo depthInfo{
        .extent = { 1920, 1080, 1 },
        .usage = ImageUsage::DepthStencil,
        .domain = MemoryDomain::Device,
        .imageType = ImageType::e2D,
        .format = aph::Format::D32,
    };

    // Geometry Pass outputs (G-buffer)
    geometryPass->setColorOut("PositionBuffer", { .createInfo = colorInfo });
    geometryPass->setColorOut("NormalBuffer", { .createInfo = colorInfo });
    geometryPass->setColorOut("AlbedoBuffer", { .createInfo = colorInfo });
    geometryPass->setDepthStencilOut("DepthBuffer", { .createInfo = depthInfo });

    // Compute Pass - Some computation on position data
    computePass->addTextureOut("ComputedData");
    computePass->addTextureIn("PositionBuffer"); // Reads from Geometry Pass

    // Transfer Pass - Upload some data
    transferPass->addBufferOut("TransferBuffer");

    // Lighting Pass - Deferred shading
    lightingPass->addTextureIn("PositionBuffer"); // Reads position from Geometry Pass
    lightingPass->addTextureIn("NormalBuffer"); // Reads normals from Geometry Pass
    lightingPass->addTextureIn("AlbedoBuffer"); // Reads albedo from Geometry Pass
    lightingPass->addBufferIn("TransferBuffer", {}, BufferUsage::Storage); // Reads data from Transfer Pass
    lightingPass->setColorOut("LightingResult", { .createInfo = colorInfo });

    // Post Process Pass - Final image processing
    postProcessPass->addTextureIn("LightingResult"); // Reads from Lighting Pass
    postProcessPass->addTextureIn("ComputedData"); // Reads from Compute Pass
    postProcessPass->setColorOut("FinalImage", { .createInfo = colorInfo });

    // Set the back buffer
    m_renderGraph->setBackBuffer("FinalImage");

    // Register callbacks (these won't be executed in dry run mode, you could omit them either)
    geometryPass->recordExecute([](aph::vk::CommandBuffer*) {});
    computePass->recordExecute([](aph::vk::CommandBuffer*) {});
    transferPass->recordExecute([](aph::vk::CommandBuffer*) {});
    lightingPass->recordExecute([](aph::vk::CommandBuffer*) {});
    postProcessPass->recordExecute([](aph::vk::CommandBuffer*) {});

    // Build the graph - performs dependency analysis and topological sorting
    m_renderGraph->build();

    // Execute the graph - in dry run mode, this will only output the execution order
    m_renderGraph->execute();

    // Export the visualization
    exportRenderGraphToDot("complex_render_graph.dot");
}

void RenderGraphVisualizer::exportRenderGraphToDot(const std::string& filename)
{
    // Export the graph visualization to GraphViz format
    std::string dotGraph = m_renderGraph->exportToGraphviz();

    auto& fs = APH_DEFAULT_FILESYSTEM;
    fs.writeStringToFile(filename, dotGraph);

    APP_LOG_INFO("\nSaved render graph visualization to '%s'", filename);
    APP_LOG_INFO("You can visualize this file using:");
    APP_LOG_INFO("  1. Online tools like https://dreampuf.github.io/GraphvizOnline/");
    APP_LOG_INFO("  2. GraphViz command line: 'dot -Tpng %s -o %s.png", filename, filename);
}

void RenderGraphVisualizer::load()
{
    APH_PROFILER_SCOPE();
    // Nothing to load in this example
}

void RenderGraphVisualizer::loop()
{
    APH_PROFILER_SCOPE();
}

void RenderGraphVisualizer::unload()
{
    APH_PROFILER_SCOPE();
    // Nothing to unload in this example
}

void RenderGraphVisualizer::finish()
{
    APH_PROFILER_SCOPE();
    // Clean up the render graph
    m_renderGraph.reset();
}

int main(int argc, char** argv)
{
    RenderGraphVisualizer app;

    // Parse command line arguments
    auto result = app.getOptions()
                      .setWindowWidth(800)
                      .setWindowHeight(600)
                      .setVsync(true)
                      .addCLICallback("--example-type", [&app](std::string_view value) { app.setExampleType(value); })
                      .parse(argc, argv);

    APH_VR(result);

    app.run();

    return 0;
}
