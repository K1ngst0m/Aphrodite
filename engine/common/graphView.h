#pragma once

#include "common/hash.h"
#include <concepts>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

namespace aph
{

// Forward declarations
class GraphNode;
class GraphEdge;

struct GraphColor
{
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;

    std::string toString() const;

    static GraphColor fromHex(const std::string& hexCode);
};

// Enum for node shapes
enum class NodeShape
{
    Box,
    Circle,
    Diamond,
    Ellipse,
    Hexagon,
    Octagon,
    Rectangle,
    RoundedBox,
    Triangle,
};

// Enum for graph direction
enum class GraphDirection
{
    LeftToRight,
    RightToLeft,
    TopToBottom,
    BottomToTop,
};

// Enum for edge style
enum class EdgeStyle
{
    Solid,
    Dashed,
    Dotted,
    Bold,
};

// Main class for graph visualization
class GraphVisualizer
{
public:
    GraphVisualizer();
    ~GraphVisualizer();

    // Graph-level settings
    void setName(const std::string& name);
    void setDirection(GraphDirection direction);
    void setFontName(const std::string& fontName);
    void setNodeSeparation(float nodeSep);
    void setRankSeparation(float rankSep);

    // Node management
    GraphNode* addNode(const std::string& id);
    GraphNode* getNode(const std::string& id);

    // Edge management
    GraphEdge* addEdge(const std::string& fromNodeId, const std::string& toNodeId);
    GraphEdge* addEdge(GraphNode* fromNode, GraphNode* toNode);

    // Default styles
    void setDefaultNodeStyle(NodeShape shape, const GraphColor& fillColor, const GraphColor& borderColor);
    void setDefaultEdgeStyle(EdgeStyle style, const GraphColor& color, float thickness);

    // Export to DOT format
    std::string exportToDot() const;

private:
    std::string m_name;
    GraphDirection m_direction;
    std::string m_fontName;
    float m_nodeSeparation;
    float m_rankSeparation;

    // Default styles
    NodeShape m_defaultNodeShape;
    GraphColor m_defaultNodeFillColor;
    GraphColor m_defaultNodeBorderColor;
    EdgeStyle m_defaultEdgeStyle;
    GraphColor m_defaultEdgeColor;
    float m_defaultEdgeThickness;

    // Nodes and edges collections
    HashMap<std::string, GraphNode*> m_nodes;
    std::vector<GraphEdge*> m_edges;

    friend class GraphNode;
    friend class GraphEdge;
    // Helper to convert NodeShape to string
    static std::string nodeShapeToString(NodeShape shape);
    // Helper to convert GraphDirection to string
    static std::string graphDirectionToString(GraphDirection direction);
    // Helper to convert EdgeStyle to string
    static std::string edgeStyleToString(EdgeStyle style);
};

// Represents a node in the graph
class GraphNode
{
public:
    GraphNode(const std::string& id);
    ~GraphNode();

    // Node attributes
    void setLabel(const std::string& label);
    void setShape(NodeShape shape);
    void setFillColor(const GraphColor& color);
    void setBorderColor(const GraphColor& color);
    void setFontName(const std::string& fontName);
    void setFontSize(int fontSize);

    // HTML-like table for structured content
    void beginTable();
    void addTableRow(const std::string& leftContent, const std::string& rightContent, bool isHeader = false);
    void endTable();

    // Get the node ID
    const std::string& getId() const
    {
        return m_id;
    }

private:
    friend class GraphVisualizer;

    std::string m_id;
    std::string m_label;
    NodeShape m_shape;
    GraphColor m_fillColor;
    GraphColor m_borderColor;
    std::string m_fontName;
    int m_fontSize;

    // Table content
    bool m_hasTable;
    std::stringstream m_tableContent;

    // Export to DOT format (called by GraphVisualizer)
    std::string toDot() const;
};

// Represents an edge in the graph
class GraphEdge
{
public:
    GraphEdge(GraphNode* fromNode, GraphNode* toNode);
    ~GraphEdge();

    // Edge attributes
    void setLabel(const std::string& label);
    void setColor(const GraphColor& color);
    void setStyle(EdgeStyle style);
    void setThickness(float thickness);
    void setFontName(const std::string& fontName);
    void setFontSize(int fontSize);

private:
    friend class GraphVisualizer;

    GraphNode* m_fromNode;
    GraphNode* m_toNode;
    std::string m_label;
    GraphColor m_color;
    EdgeStyle m_style;
    float m_thickness;
    std::string m_fontName;
    int m_fontSize;

    // Export to DOT format (called by GraphVisualizer)
    std::string toDot() const;
};

} // namespace aph
