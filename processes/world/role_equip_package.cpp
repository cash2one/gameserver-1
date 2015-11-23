#include "role.h"
#include "world.h"
#include "scene.h"
#include "role_equip_package.h"
#include "suit_config.h"

#include "water/common/commdef.h"
#include "water/componet/logger.h"
#include "water/process/process_id.h"

#include "protocol/rawmsg/rawmsg_manager.h"

#include "protocol/rawmsg/private/package.h"
#include "protocol/rawmsg/private/package.codedef.private.h"

#include "protocol/rawmsg/public/package.h"
#include "protocol/rawmsg/public/package.codedef.public.h"

namespace world{

RoleEquipPackage::RoleEquipPackage(SceneItemType sceneItem, PackageType packageType, uint16_t totalCellNum, uint16_t unlockNum, const std::vector<CellInfo>& cellVec)
: EquipPackage(sceneItem, packageType, totalCellNum, unlockNum, cellVec)
{
}


bool RoleEquipPackage::checkPutObjByCell(Object::Ptr obj, uint16_t cell, uint16_t num, Bind bind)
{
	Role::Ptr role = std::static_pointer_cast<Role>(getOwner());
	if(role == nullptr)
		return false;

	if(nullptr == obj || 1 != num || bind == Bind::none)
		return false;

	if(Package::getObjByCell(cell) != nullptr)
		return false;
	
	if(getObjChildTypeByCell(cell) != obj->childType())
	{
		role->sendSysChat("不可穿戴");
		return false;
	}

	if(role->level() < obj->level())
	{
		role->sendSysChat("等级不足");
		return false;
	}

	if(role->turnLifeLevel() < obj->turnLife())
	{
		role->sendSysChat("转生等级不足");
		return false;
	}

	if(Job::none != obj->job())
	{
		if(role->job() != obj->job())
		{
			role->sendSysChat("职业不符");
			return false;
		}
	}

	if(Sex::none != obj->sex())
	{
		if(role->sex() != obj->sex())
		{
			role->sendSysChat("性别不符");
			return false;
		}
	}

	return true;
}

void RoleEquipPackage::sendAllAttributeToMe()
{
	calculateAllAttribute();

	Role::Ptr role = std::static_pointer_cast<Role>(getOwner());
	if(role == nullptr)
		return;

	role->sendMainToMe();
	return;
}

void RoleEquipPackage::updateAllEquipSkills()
{
	Role::Ptr role = std::static_pointer_cast<Role>(getOwner());
	if(role == nullptr)
		return;
	
	resetSuitPropList();

	std::vector<uint32_t> skillVec;
	const std::vector<CellInfo>& cellVec = Package::getCellVec();
	for(auto iter = cellVec.begin(); iter != cellVec.end(); ++iter)
	{
		if(iter->objPtr == nullptr)
			continue;

		uint32_t skillId = iter->objPtr->skillId();
		if(0 == skillId)
			continue;

		skillVec.push_back(skillId);
		LOG_TRACE("背包, 角色装备包, 极品装备触发的技能, objId={}, tplId={}, item={}, cell={}, skillId={}, role=({}, {}, {})",
				  iter->objId, iter->objPtr->tplId(), iter->item, iter->cell, skillId,
				  role->name(), role->id(), role->account());
	}

	const auto& skillOfSuitMap = EquipPackage::getSkillOfSuitMap();
	for(auto pos = skillOfSuitMap.begin(); pos != skillOfSuitMap.end(); ++pos)
	{
		if(pos->second.empty())
			continue;
		
		for(uint32_t i = 0; i < pos->second.size(); i++)
		{
			uint32_t skillId = pos->second[i];
			skillVec.push_back(skillId);
			LOG_TRACE("背包, 角色装备包, 套装属性, suitId={}, skillId={}", pos->first, skillId);
		}
	}

	if(skillVec.empty())
		return;

	role->m_skillM.putEquipPassiveSkill(skillVec);
	return;
}

//将装备包显示外观的物品列表发到九屏
void RoleEquipPackage::sendObjListToNine()
{
	Role::Ptr role = std::static_pointer_cast<Role>(getOwner());
	if(role == nullptr)
	{
		LOG_ERROR("背包, 角色装备包, 外观物品列表, Role::WPtr失效, 发送给九屏失败, packageType={}",
				  getPackageType());
		return;
	}

	Scene::Ptr s = role->scene();
	if(s == nullptr)
		return;

	PublicRaw::RetPackageObjListToNine send;
	send.id = role->id();
	send.packageType = getPackageType();
	send.tplId[0] = getTplIdByCell(0);
	send.tplId[1] = getTplIdByCell(2);
	send.tplId[2] = getTplIdByCell(9);
	
	s->sendCmdTo9(RAWMSG_CODE_PUBLIC(RetPackageObjListToNine),
				  &send, sizeof(send), role->pos());
	LOG_TRACE("背包, 角色装备包, 外观物品列表, 发送给九屏, 总体, roleId={}, packageType={}, objListSize={}, tplId=({}, {}, {}), role=({}, {}, {})",
			  send.id, send.packageType, send.objListSize, 
			  send.tplId[0], send.tplId[1], send.tplId[2],
			  role->name(), role->id(), role->account());

	return; 
}



}