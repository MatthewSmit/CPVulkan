#pragma once
#include <Base.h>

#include <vector>

class PipelineLayout;

struct VertexInputState
{
	std::vector<VkVertexInputBindingDescription> VertexBindingDescriptions;
	std::vector<VkVertexInputAttributeDescription> VertexAttributeDescriptions;
};

struct InputAssemblyState
{
	VkPrimitiveTopology Topology;
	bool PrimitiveRestartEnable;
};

struct TessellationState
{
	uint32_t PatchControlPoints;
};

struct ViewportState
{
	std::vector<VkViewport> Viewports;
	std::vector<VkRect2D> Scissors;
};

struct RasterizationState
{
	bool DepthClampEnable;
	bool RasterizerDiscardEnable;
	VkPolygonMode PolygonMode;
	VkCullModeFlags CullMode;
	VkFrontFace FrontFace;
	bool DepthBiasEnable;
	float DepthBiasConstantFactor;
	float DepthBiasClamp;
	float DepthBiasSlopeFactor;
	float LineWidth;

#if defined(VK_EXT_line_rasterization)
	VkLineRasterizationModeEXT LineRasterizationMode;
	bool StippledLineEnable;
	uint32_t LineStippleFactor;
	uint32_t LineStipplePattern;
#endif
};

struct MultisampleState
{
	VkSampleCountFlagBits RasterizationSamples;
	bool SampleShadingEnable;
	float MinSampleShading;
	bool AlphaToCoverageEnable;
	bool AlphaToOneEnable;
	uint64_t SampleMask;
};

struct DepthStencilState
{
	bool DepthTestEnable;
	bool DepthWriteEnable;
	VkCompareOp DepthCompareOp;
	bool DepthBoundsTestEnable;
	bool StencilTestEnable;
	VkStencilOpState Front;
	VkStencilOpState Back;
	float MinDepthBounds;
	float MaxDepthBounds;
};

struct ColourBlendState
{
	bool LogicOpEnable;
	VkLogicOp LogicOp;
	std::vector<VkPipelineColorBlendAttachmentState> Attachments;
	float BlendConstants[4];
};

struct DynamicState
{
	bool DynamicViewport;
	bool DynamicScissor;
	bool DynamicLineWidth;
	bool DynamicDepthBias;
	bool DynamicBlendConstants;
	bool DynamicDepthBounds;
	bool DynamicStencilCompareMask;
	bool DynamicStencilWriteMask;
	bool DynamicStencilReference;
	bool DynamicViewportWScaling;
	bool DynamicDiscardRectangle;
	bool DynamicSampleLocations;
	bool DynamicViewportShadingRatePalette;
	bool DynamicViewportCoarseSampleOrder;
	bool DynamicExclusiveScissor;
	bool DynamicLineStipple;
};

struct AttachmentDescription
{
	VkAttachmentDescriptionFlags flags;
	VkFormat format;
	VkSampleCountFlagBits samples;
	VkAttachmentLoadOp loadOp;
	VkAttachmentStoreOp storeOp;
	VkAttachmentLoadOp stencilLoadOp;
	VkAttachmentStoreOp stencilStoreOp;
	VkImageLayout initialLayout;
	VkImageLayout finalLayout;
};

struct AttachmentReference
{
	uint32_t attachment;
	VkImageLayout layout;
	VkImageAspectFlags aspectMask;
};

struct SubpassDescription
{
	VkSubpassDescriptionFlags flags;
	VkPipelineBindPoint pipelineBindPoint;
	uint32_t viewMask;
	std::vector<AttachmentReference> inputAttachments;
	std::vector<AttachmentReference> colourAttachments;
	std::vector<AttachmentReference> resolveAttachments;
	AttachmentReference depthStencilAttachment;
	std::vector<uint32_t> preserveAttachments;
};

struct SubpassDependency
{
	uint32_t srcSubpass;
	uint32_t dstSubpass;
	VkPipelineStageFlags srcStageMask;
	VkPipelineStageFlags dstStageMask;
	VkAccessFlags srcAccessMask;
	VkAccessFlags dstAccessMask;
	VkDependencyFlags dependencyFlags;
	int32_t viewOffset;
};

class GraphicsPipelineStateStorage
{
public:
	[[nodiscard]] virtual const PipelineLayout* getLayout() const = 0;
	
	[[nodiscard]] virtual const VertexInputState& getVertexInputState() const = 0;
	[[nodiscard]] virtual const InputAssemblyState& getInputAssemblyState() const = 0;
	[[nodiscard]] virtual const TessellationState& getTessellationState() const = 0;
	[[nodiscard]] virtual const ViewportState& getViewportState() const = 0;
	[[nodiscard]] virtual const RasterizationState& getRasterizationState() const = 0;
	[[nodiscard]] virtual const MultisampleState& getMultisampleState() const = 0;
	[[nodiscard]] virtual const DepthStencilState& getDepthStencilState() const = 0;
	[[nodiscard]] virtual const ColourBlendState& getColourBlendState() const = 0;
	[[nodiscard]] virtual const DynamicState& getDynamicState() const = 0;

	[[nodiscard]] virtual const std::vector<AttachmentDescription>& getAttachments() const = 0;
	[[nodiscard]] virtual const std::vector<SubpassDescription>& getSubpasses() const = 0;
	[[nodiscard]] virtual const std::vector<SubpassDependency>& getDependencies() const = 0;
};