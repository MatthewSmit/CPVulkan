#include "DescriptorSet.h"

VkResult DescriptorSet::Create(VkDescriptorPool descriptorPool, VkDescriptorSetLayout pSetLayout, VkDescriptorSet* pDescriptorSet)
{
	auto descriptorSet = Allocate<DescriptorSet>(nullptr);

	*pDescriptorSet = reinterpret_cast<VkDescriptorSet>(descriptorSet);
	return VK_SUCCESS;
}
