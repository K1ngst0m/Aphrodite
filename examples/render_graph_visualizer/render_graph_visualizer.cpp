#include "render_graph_visualizer.h"

RenderGraphVisualizer::RenderGraphVisualizer()
    : aph::App("Render Graph Visualizer")
{
}

RenderGraphVisualizer::~RenderGraphVisualizer()
{
    if (m_renderGraph)
    {
        aph::RenderGraph::Destroy(m_renderGraph);
        m_renderGraph = nullptr;
    }
}

void RenderGraphVisualizer::init()
{
    APH_PROFILER_SCOPE();

    // Create a new render graph in dry run mode (no GPU operations)
    auto result = aph::RenderGraph::CreateDryRun();
    aph::VerifyExpected(result);
    m_renderGraph = result.value();

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
    auto* mainPass        = m_renderGraph->createPass("MainPass", aph::QueueType::Graphics);
    auto* postProcessPass = m_renderGraph->createPass("PostProcessPass", aph::QueueType::Graphics);

    // Create image resources
    aph::vk::ImageCreateInfo colorInfo{
        .extent    = {1920, 1080, 1},
        .usage     = aph::ImageUsage::ColorAttachment,
        .domain    = aph::MemoryDomain::Device,
        .imageType = aph::ImageType::e2D,
        .format    = aph::Format::RGBA8_UNORM,
    };

    aph::vk::ImageCreateInfo depthInfo{
        .extent    = {1920, 1080, 1},
        .usage     = aph::ImageUsage::DepthStencil,
        .domain    = aph::MemoryDomain::Device,
        .imageType = aph::ImageType::e2D,
        .format    = aph::Format::D32,
    };

    // Configure passes
    mainPass->setColorOut("SceneColor", {.createInfo = colorInfo});
    mainPass->setDepthStencilOut("SceneDepth", {.createInfo = depthInfo});

    postProcessPass->addTextureIn("SceneColor");
    postProcessPass->setColorOut("FinalColor", {.createInfo = colorInfo});

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
    auto geomGroup     = m_renderGraph->createPassGroup("GeometryGroup");
    auto computeGroup  = m_renderGraph->createPassGroup("ComputeGroup");
    auto lightingGroup = m_renderGraph->createPassGroup("LightingGroup");

    // Create image resources
    aph::vk::ImageCreateInfo colorInfo{
        .extent    = {1920, 1080, 1},
        .usage     = aph::ImageUsage::ColorAttachment,
        .domain    = aph::MemoryDomain::Device,
        .imageType = aph::ImageType::e2D,
        .format    = aph::Format::RGBA8_UNORM,
    };

    aph::vk::ImageCreateInfo depthInfo{
        .extent    = {1920, 1080, 1},
        .usage     = aph::ImageUsage::DepthStencil,
        .domain    = aph::MemoryDomain::Device,
        .imageType = aph::ImageType::e2D,
        .format    = aph::Format::D32,
    };

    // Create a Geometry Pass
    auto* geometryPass = geomGroup.addPass("Geometry Pass", aph::QueueType::Graphics);
    geometryPass->configure()
        .attachment("PositionBuffer", {.createInfo = colorInfo}, false)
        .attachment("NormalBuffer", {.createInfo = colorInfo}, false)
        .attachment("AlbedoBuffer", {.createInfo = colorInfo}, false)
        .attachment("DepthBuffer", {.createInfo = depthInfo}, true)
        .execute([](aph::vk::CommandBuffer*) {})
        .build();

    // Create a Compute Pass
    auto* computePass = computeGroup.addPass("Compute Pass", aph::QueueType::Compute);
    computePass->configure()
        .output("ComputedData", aph::ImageUsage::Storage)
        .resource("PositionBuffer", nullptr, aph::ImageUsage::Sampled)
        .execute([](aph::vk::CommandBuffer*) {})
        .build();

    // Create a Transfer Pass
    auto* transferPass = m_renderGraph->createPass("Transfer Pass", aph::QueueType::Transfer);
    transferPass->configure()
        .output("TransferBuffer", aph::BufferUsage::Storage)
        .execute([](aph::vk::CommandBuffer*) {})
        .build();

    // Create a Lighting Pass
    auto* lightingPass = lightingGroup.addPass("Lighting Pass", aph::QueueType::Graphics);
    lightingPass->configure()
        .resource("PositionBuffer", nullptr, aph::ImageUsage::Sampled)
        .resource("NormalBuffer", nullptr, aph::ImageUsage::Sampled)
        .resource("AlbedoBuffer", nullptr, aph::ImageUsage::Sampled)
        .resource("TransferBuffer", nullptr, aph::BufferUsage::Storage)
        .output("LightingResult", aph::ImageUsage::Storage)
        .execute([](aph::vk::CommandBuffer*) {})
        .build();

    // Create a Post-Process Pass
    auto* postProcessPass = m_renderGraph->createPass("Post-Process Pass", aph::QueueType::Graphics);
    postProcessPass->configure()
        .resource("LightingResult", nullptr, aph::ImageUsage::Sampled)
        .resource("ComputedData", nullptr, aph::ImageUsage::Sampled)
        .attachment("FinalOutput", {.createInfo = colorInfo}, false)
        .execute([](aph::vk::CommandBuffer*) {})
        .build();

    // Demonstrate conditional execution
    computePass->setExecutionCondition(
        []()
        {
            // This is just an example condition - in reality would depend on app state
            return true;
        });

    // Set the back buffer
    m_renderGraph->setBackBuffer("FinalOutput");

    // Build and execute the graph
    m_renderGraph->build();
    m_renderGraph->execute();

    // Export the visualization
    exportRenderGraphToDot("complex_render_graph.dot");
}

void RenderGraphVisualizer::exportRenderGraphToDot(const std::string& filename)
{
    // Export the graph visualization to GraphViz format
    std::string dotGraph = m_renderGraph->exportToGraphviz();

    auto& fs         = APH_DEFAULT_FILESYSTEM;
    auto writeResult = fs.writeStringToFile(filename, dotGraph);
    if (!writeResult.success())
    {
        APP_LOG_ERR("Failed to save render graph visualization: %s", writeResult.toString().data());
        return;
    }

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
    // Graph will be cleaned up in destructor
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

    APH_VERIFY_RESULT(result);

    app.run();

    return 0;
}
