#include "graphView.h"

#include <iomanip>
#include <sstream>

namespace aph
{

//------------------------------------------------------------------------------
// GraphColor implementation
//------------------------------------------------------------------------------

std::string GraphColor::toString() const
{
    std::stringstream ss;

    // Format: #RRGGBBAA or #RRGGBB if alpha is 1.0
    ss << "#";
    ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(r * 255);
    ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(g * 255);
    ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(b * 255);

    if (a < 1.0f)
    {
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(a * 255);
    }

    return ss.str();
}

GraphColor GraphColor::fromHex(const std::string& hexCode)
{
    GraphColor color;

    // Remove '#' if present
    std::string code = hexCode;
    if (code[0] == '#')
    {
        code = code.substr(1);
    }

    // Parse the hex code
    int r, g, b, a = 255;
    if (code.length() >= 6)
    {
        std::stringstream ss;
        ss << std::hex << code.substr(0, 2);
        ss >> r;
        ss.clear();

        ss << std::hex << code.substr(2, 2);
        ss >> g;
        ss.clear();

        ss << std::hex << code.substr(4, 2);
        ss >> b;

        if (code.length() >= 8)
        {
            ss.clear();
            ss << std::hex << code.substr(6, 2);
            ss >> a;
        }

        color.r = r / 255.0f;
        color.g = g / 255.0f;
        color.b = b / 255.0f;
        color.a = a / 255.0f;
    }

    return color;
}

//------------------------------------------------------------------------------
// GraphNode implementation
//------------------------------------------------------------------------------

GraphNode::GraphNode(const std::string& id)
    : m_id(id)
    , m_shape(NodeShape::Box)
    , m_fillColor({1.0f, 1.0f, 1.0f, 1.0f})
    , m_borderColor({0.0f, 0.0f, 0.0f, 1.0f})
    , m_fontName("Arial")
    , m_fontSize(10)
    , m_hasTable(false)
{
}

GraphNode::~GraphNode()
{
}

void GraphNode::setLabel(const std::string& label)
{
    m_label    = label;
    m_hasTable = false; // Setting a plain label clears any table
}

void GraphNode::setShape(NodeShape shape)
{
    m_shape = shape;
}

void GraphNode::setFillColor(const GraphColor& color)
{
    m_fillColor = color;
}

void GraphNode::setBorderColor(const GraphColor& color)
{
    m_borderColor = color;
}

void GraphNode::setFontName(const std::string& fontName)
{
    m_fontName = fontName;
}

void GraphNode::setFontSize(int fontSize)
{
    m_fontSize = fontSize;
}

void GraphNode::beginTable()
{
    m_hasTable = true;
    m_tableContent.str("");
    m_tableContent << "<TABLE BORDER=\"0\" CELLBORDER=\"0\" CELLSPACING=\"2\" CELLPADDING=\"2\">\n";
}

void GraphNode::addTableRow(const std::string& leftContent, const std::string& rightContent, bool isHeader)
{
    if (!m_hasTable)
    {
        beginTable();
    }

    m_tableContent << "    <TR>";

    if (isHeader)
    {
        m_tableContent << "<TD COLSPAN=\"2\"><B>" << leftContent << "</B></TD>";
    }
    else
    {
        m_tableContent << "<TD ALIGN=\"LEFT\">" << leftContent << "</TD>";
        m_tableContent << "<TD ALIGN=\"LEFT\">" << rightContent << "</TD>";
    }

    m_tableContent << "</TR>\n";
}

void GraphNode::endTable()
{
    if (m_hasTable)
    {
        m_tableContent << "  </TABLE>";
    }
}

std::string GraphNode::toDot() const
{
    std::stringstream ss;

    ss << "  \"" << m_id << "\" [";

    // Shape
    ss << "shape=" << GraphVisualizer::nodeShapeToString(m_shape) << ", ";

    // Colors
    ss << "fillcolor=\"" << m_fillColor.toString() << "\", ";
    ss << "color=\"" << m_borderColor.toString() << "\", ";

    // Font
    ss << "fontname=\"" << m_fontName << "\", ";
    ss << "fontsize=" << m_fontSize << ", ";

    // Style (always filled for nodes)
    ss << "style=\"rounded,filled\", ";

    // Label (either plain text or HTML table)
    if (m_hasTable)
    {
        ss << "label=<" << m_tableContent.str() << ">";
    }
    else if (!m_label.empty())
    {
        ss << "label=\"" << m_label << "\"";
    }

    ss << "];\n";

    return ss.str();
}

//------------------------------------------------------------------------------
// GraphEdge implementation
//------------------------------------------------------------------------------

GraphEdge::GraphEdge(GraphNode* fromNode, GraphNode* toNode)
    : m_fromNode(fromNode)
    , m_toNode(toNode)
    , m_color({0.0f, 0.0f, 0.0f, 1.0f})
    , m_style(EdgeStyle::Solid)
    , m_thickness(1.0f)
    , m_fontName("Arial")
    , m_fontSize(9)
{
}

GraphEdge::~GraphEdge()
{
}

void GraphEdge::setLabel(const std::string& label)
{
    m_label = label;
}

void GraphEdge::setColor(const GraphColor& color)
{
    m_color = color;
}

void GraphEdge::setStyle(EdgeStyle style)
{
    m_style = style;
}

void GraphEdge::setThickness(float thickness)
{
    m_thickness = thickness;
}

void GraphEdge::setFontName(const std::string& fontName)
{
    m_fontName = fontName;
}

void GraphEdge::setFontSize(int fontSize)
{
    m_fontSize = fontSize;
}

std::string GraphEdge::toDot() const
{
    std::stringstream ss;

    ss << "  \"" << m_fromNode->getId() << "\" -> \"" << m_toNode->getId() << "\" [";

    // Label
    if (!m_label.empty())
    {
        ss << "label=\"" << m_label << "\", ";
    }

    // Color and thickness
    ss << "color=\"" << m_color.toString() << "\", ";
    ss << "penwidth=" << m_thickness << ", ";

    // Style
    ss << "style=" << GraphVisualizer::edgeStyleToString(m_style) << ", ";

    // Font
    ss << "fontname=\"" << m_fontName << "\", ";
    ss << "fontsize=" << m_fontSize;

    ss << "];\n";

    return ss.str();
}

//------------------------------------------------------------------------------
// GraphVisualizer implementation
//------------------------------------------------------------------------------

GraphVisualizer::GraphVisualizer()
    : m_name("G")
    , m_direction(GraphDirection::LeftToRight)
    , m_fontName("Arial")
    , m_nodeSeparation(0.8f)
    , m_rankSeparation(1.0f)
    , m_defaultNodeShape(NodeShape::RoundedBox)
    , m_defaultNodeFillColor({0.9f, 0.9f, 0.9f, 1.0f})
    , m_defaultNodeBorderColor({0.2f, 0.2f, 0.2f, 1.0f})
    , m_defaultEdgeStyle(EdgeStyle::Solid)
    , m_defaultEdgeColor({0.0f, 0.0f, 0.0f, 1.0f})
    , m_defaultEdgeThickness(1.0f)
{
}

GraphVisualizer::~GraphVisualizer()
{
    // Cleanup allocated nodes
    for (auto& [id, node] : m_nodes)
    {
        delete node;
    }

    // Cleanup allocated edges
    for (auto& edge : m_edges)
    {
        delete edge;
    }
}

void GraphVisualizer::setName(const std::string& name)
{
    m_name = name;
}

void GraphVisualizer::setDirection(GraphDirection direction)
{
    m_direction = direction;
}

void GraphVisualizer::setFontName(const std::string& fontName)
{
    m_fontName = fontName;
}

void GraphVisualizer::setNodeSeparation(float nodeSep)
{
    m_nodeSeparation = nodeSep;
}

void GraphVisualizer::setRankSeparation(float rankSep)
{
    m_rankSeparation = rankSep;
}

GraphNode* GraphVisualizer::addNode(const std::string& id)
{
    if (m_nodes.contains(id))
    {
        return m_nodes[id];
    }

    auto* node  = new GraphNode(id);
    m_nodes[id] = node;

    // Apply default node style
    node->setShape(m_defaultNodeShape);
    node->setFillColor(m_defaultNodeFillColor);
    node->setBorderColor(m_defaultNodeBorderColor);

    return node;
}

GraphNode* GraphVisualizer::getNode(const std::string& id)
{
    if (m_nodes.contains(id))
    {
        return m_nodes[id];
    }
    return nullptr;
}

GraphEdge* GraphVisualizer::addEdge(const std::string& fromNodeId, const std::string& toNodeId)
{
    auto* fromNode = getNode(fromNodeId);
    auto* toNode   = getNode(toNodeId);

    if (!fromNode)
    {
        fromNode = addNode(fromNodeId);
    }

    if (!toNode)
    {
        toNode = addNode(toNodeId);
    }

    return addEdge(fromNode, toNode);
}

GraphEdge* GraphVisualizer::addEdge(GraphNode* fromNode, GraphNode* toNode)
{
    auto* edge = new GraphEdge(fromNode, toNode);
    m_edges.push_back(edge);

    // Apply default edge style
    edge->setStyle(m_defaultEdgeStyle);
    edge->setColor(m_defaultEdgeColor);
    edge->setThickness(m_defaultEdgeThickness);

    return edge;
}

void GraphVisualizer::setDefaultNodeStyle(NodeShape shape, const GraphColor& fillColor, const GraphColor& borderColor)
{
    m_defaultNodeShape       = shape;
    m_defaultNodeFillColor   = fillColor;
    m_defaultNodeBorderColor = borderColor;
}

void GraphVisualizer::setDefaultEdgeStyle(EdgeStyle style, const GraphColor& color, float thickness)
{
    m_defaultEdgeStyle     = style;
    m_defaultEdgeColor     = color;
    m_defaultEdgeThickness = thickness;
}

std::string GraphVisualizer::exportToDot() const
{
    std::stringstream dot;

    // Start digraph definition
    dot << "digraph " << m_name << " {\n";

    // Graph attributes
    dot << "  // Graph styling\n";
    dot << "  graph [rankdir=" << graphDirectionToString(m_direction) << ", ";
    dot << "fontname=\"" << m_fontName << "\", ";
    dot << "nodesep=" << m_nodeSeparation << ", ";
    dot << "ranksep=" << m_rankSeparation << "];\n";

    // Default node attributes
    dot << "  node [fontname=\"" << m_fontName << "\", ";
    dot << "shape=" << nodeShapeToString(m_defaultNodeShape) << ", ";
    dot << "style=\"rounded,filled\"];\n";

    // Default edge attributes
    dot << "  edge [fontname=\"" << m_fontName << "\"];\n\n";

    // Add nodes
    dot << "  // Nodes\n";
    for (const auto& [id, node] : m_nodes)
    {
        dot << node->toDot();
    }

    dot << "\n  // Edges\n";
    for (const auto& edge : m_edges)
    {
        dot << edge->toDot();
    }

    // End graph
    dot << "}\n";

    return dot.str();
}

std::string GraphVisualizer::nodeShapeToString(NodeShape shape)
{
    switch (shape)
    {
    case NodeShape::Box:
        return "box";
    case NodeShape::Circle:
        return "circle";
    case NodeShape::Diamond:
        return "diamond";
    case NodeShape::Ellipse:
        return "ellipse";
    case NodeShape::Hexagon:
        return "hexagon";
    case NodeShape::Octagon:
        return "octagon";
    case NodeShape::Rectangle:
        return "rectangle";
    case NodeShape::RoundedBox:
        return "box";
    case NodeShape::Triangle:
        return "triangle";
    default:
        return "box";
    }
}

std::string GraphVisualizer::graphDirectionToString(GraphDirection direction)
{
    switch (direction)
    {
    case GraphDirection::LeftToRight:
        return "LR";
    case GraphDirection::RightToLeft:
        return "RL";
    case GraphDirection::TopToBottom:
        return "TB";
    case GraphDirection::BottomToTop:
        return "BT";
    default:
        return "LR";
    }
}

std::string GraphVisualizer::edgeStyleToString(EdgeStyle style)
{
    switch (style)
    {
    case EdgeStyle::Solid:
        return "solid";
    case EdgeStyle::Dashed:
        return "dashed";
    case EdgeStyle::Dotted:
        return "dotted";
    case EdgeStyle::Bold:
        return "bold";
    default:
        return "solid";
    }
}

} // namespace aph
