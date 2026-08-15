# LxARPG

> 基于 Unreal Engine 5.6 开发的模块化动作角色扮演游戏项目。  
> A modular action RPG project built with Unreal Engine 5.6.

[中文](#中文说明) | [English](#english)

---

## 中文说明

### 项目简介

LxARPG 是一个使用 **Unreal Engine 5.6**、**C++** 与**蓝图**共同开发的第三人称 ARPG 项目。项目以组件化和数据驱动为主要设计方向，围绕角色战斗、技能组合、属性成长、物品管理、AI 行为、场景交互及游戏 UI 搭建可扩展的玩法框架。

当前项目仍处于持续开发阶段，主要用于 ARPG 核心玩法和通用系统的设计、实现与验证。

### 主要模块

- **角色与控制**：玩家角色、AI 角色、输入、移动、瞄准、状态、生命周期及动画处理。
- **战斗系统**：近战攻击、格挡、伤害计算流程、暴击、防御、护盾与生命结算。
- **技能系统**：技能施放、技能背包及可组合的技能单元，支持近战、投射物、射线、范围、光环、附着效果、召唤和触发器等形式。
- **属性、效果与 Buff**：角色基础/特殊属性、属性修改、效果传递与处理、Buff 管理及网络显示快照。
- **物品与职业**：背包、装备、消耗品、材料、技能物品、物品稀有度以及职业等级与成长效果。
- **交互系统**：通用交互节点，以及宝箱、仓库、交易、物品转移和机关等交互逻辑。
- **AI 与动画**：AI 局势和行为配置、目标关系、角色动画信号及运动分析。
- **UI 系统**：角色状态、属性、背包、技能、Buff、物品提示、快捷栏、聊天、职业、交易和世界空间信息界面。
- **数据与基础设施**：数据表管理、Gameplay Tags（游戏玩法标签）、项目设置、存档、子系统、富文本样式与通用工具。
- **场景工具**：样条墙体、样条表面、环形表面和环形柱体等程序化场景 Actor（场景对象）。

### 技术信息

- Unreal Engine 5.6
- C++ / Blueprint（蓝图）
- Enhanced Input（增强输入）
- UMG / Slate
- Gameplay Tags（游戏玩法标签）
- Modular Gameplay（模块化玩法）
- AI Module / Navigation System（AI 模块 / 导航系统）
- Procedural Mesh Component（程序化网格体组件）
- 目标平台：Windows

### 运行项目

1. 安装 Unreal Engine 5.6，并准备 Visual Studio 的 Unreal Engine C++ 开发环境。
2. 克隆仓库，在 `LxARPG.uproject` 上右键生成项目文件。
3. 使用 Visual Studio 编译 `LxARPGEditor`。
4. 打开 `LxARPG.uproject`，进入默认关卡运行项目。

> 注意：项目仍在开发中，部分功能、资源引用和数据配置可能会持续调整。仓库中第三方资源的使用需遵循其各自的授权条款。

---

## English

### Overview

LxARPG is a third-person action RPG project developed with **Unreal Engine 5.6**, **C++**, and **Blueprints**. It follows a component-based and data-driven architecture, providing an extensible foundation for character combat, composable skills, attributes and progression, inventory management, AI behavior, world interaction, and game UI.

The project is currently under active development and is primarily used to design, implement, and validate reusable ARPG gameplay systems.

### Main Modules

- **Characters & Controls**: Player and AI characters, input, movement, aiming, states, lifecycle, and animation processing.
- **Combat**: Melee attacks, blocking, configurable damage calculation, critical hits, defense, shields, and health settlement.
- **Skills**: Skill casting, skill inventory, and composable skill units for melee, projectiles, rays, areas, auras, attached effects, summons, and triggers.
- **Attributes, Effects & Buffs**: Base and specialized attributes, modifiers, effect transfer and processing, buff management, and replicated display snapshots.
- **Items & Professions**: Inventory, equipment, consumables, materials, skill items, rarity presentation, and profession progression.
- **Interaction**: A reusable interaction-node framework for chests, warehouses, trading, item transfer, and trigger mechanisms.
- **AI & Animation**: Situation-based AI behavior configuration, target relationships, animation signals, and motion analysis.
- **UI**: Character status, attributes, inventory, skills, buffs, tooltips, shortcut bar, chat, professions, trading, and world-space information.
- **Data & Infrastructure**: Data-table management, Gameplay Tags, project settings, save data, subsystems, rich-text styling, and shared utilities.
- **World Tools**: Procedural spline walls, spline surfaces, ring surfaces, and ring cylinders.

### Tech Stack

- Unreal Engine 5.6
- C++ / Blueprints
- Enhanced Input
- UMG / Slate
- Gameplay Tags
- Modular Gameplay
- AI Module / Navigation System
- Procedural Mesh Component
- Target platform: Windows

### Getting Started

1. Install Unreal Engine 5.6 and the Visual Studio workload for Unreal Engine C++ development.
2. Clone the repository and generate project files from `LxARPG.uproject`.
3. Build the `LxARPGEditor` target in Visual Studio.
4. Open `LxARPG.uproject` and play from the default map.

> Note: This project is under active development. Features, asset references, and data configurations may change. Third-party assets remain subject to their respective license terms.
