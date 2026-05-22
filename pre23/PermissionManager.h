// PermissionManager.h
// 权限管理器（简单 + 可扩展版本）
//
// 设计目标：
// 1. 支持“最小权限等级（>=）”判断
// 2. 支持在满足最小权限的前提下，排除某些特定角色（例外）
// 3. 专注于 UI 层权限控制（启用 / 禁用），不干扰样式本身
// 4. 尽量保持接口语义清晰、调用简单
//
#ifndef PERMISSIONMANAGER_H
#define PERMISSIONMANAGER_H

#include <QString>
#include <QHash>
#include <QWidget>
#include <QList>
#include "SessionManager.h"

class PermissionManager {
public:
    /**
     * @brief 权限等级定义
     *
     * 注意：
     * - 枚举值的大小代表权限强弱
     * - 数值越大，权限越高
     * - 所有权限判断均基于 >= 关系
     */
    enum PermissionLevel {
        Standard = 0,     // 标准用户
        QA = 1,           // QA
        Supervisor = 2,   // 主管
        Admin = 3,        // 管理员
        SuperAdmin = 4    // 超级管理员
    };

    /* =========================================================
     * 基础权限判断（仅最小权限等级）
     *
     * 规则：
     * - 当前用户权限 >= 指定权限，则返回 true
     * - 不支持例外排除
     *
     * 适用场景：
     * - “某个功能只有 Admin 及以上能用”
     * ========================================================= */
    static bool hasPermission(PermissionLevel minLevel) {
        // 从 SessionManager 中获取当前用户的权限字符串
        QString userLevelStr = SessionManager::instance().userLevel();

        // 将字符串形式的权限转换为枚举值
        PermissionLevel userLevel = static_cast<PermissionLevel>(
            levelFromString(userLevelStr)
            );

        // 判断是否满足最低权限等级
        return userLevel >= minLevel;
    }

    /* =========================================================
     * 带排除项的权限判断（新增能力）
     *
     * 规则：
     * 1. 当前用户权限必须 >= minLevel
     * 2. 当前用户权限不能出现在 excludedLevels 中
     *
     * 适用场景：
     * - “Standard 以上都可以，但 QA 不可以”
     * - “Supervisor 以上都可以，但 Admin 被限制”
     * ========================================================= */
    static bool hasPermission(
        PermissionLevel minLevel,
        const QList<PermissionLevel>& excludedLevels
        ) {
        QString userLevelStr = SessionManager::instance().userLevel();
        PermissionLevel userLevel = static_cast<PermissionLevel>(
            levelFromString(userLevelStr)
            );

        // 1. 不满足最低权限，直接拒绝
        if (userLevel < minLevel)
            return false;

        // 2. 命中排除权限列表，直接拒绝
        if (excludedLevels.contains(userLevel))
            return false;

        // 3. 满足所有规则，允许
        return true;
    }

    /* =========================================================
     * UI 控件权限控制（基础版）
     *
     * 功能：
     * - 根据权限启用 / 禁用控件
     * - 不修改控件样式（样式由 Qt Designer 决定）
     *
     * 注意：
     * - 禁用状态下，Qt 会自动使用 Designer 中设置的 Disabled 样式
     * ========================================================= */
    static void setUIByPermission(PermissionLevel minLevel, QWidget* widget) {
        if (!widget)
            return;

        bool enabled = hasPermission(minLevel);
        widget->setEnabled(enabled);
    }

    /* =========================================================
     * UI 控件权限控制（带排除规则）
     *
     * 功能：
     * - 在满足最小权限的前提下
     * - 对指定权限进行单独限制
     *
     * 示例：
     * setUIByPermission(Standard, button, { QA });
     * ========================================================= */
    static void setUIByPermission(
        PermissionLevel minLevel,
        QWidget* widget,
        const QList<PermissionLevel>& excludedLevels
        ) {
        if (!widget)
            return;

        bool enabled = hasPermission(minLevel, excludedLevels);
        widget->setEnabled(enabled);
    }

    /* =========================================================
     * 批量设置控件权限（基础版）
     *
     * 适用场景：
     * - 一组按钮、输入框权限一致
     * ========================================================= */
    static void setWidgetsByPermission(
        PermissionLevel minLevel,
        const QList<QWidget*>& widgets
        ) {
        for (QWidget* widget : widgets) {
            setUIByPermission(minLevel, widget);
        }
    }

    /* =========================================================
     * 批量设置控件权限（带排除规则）
     *
     * 适用场景：
     * - 页面级别的统一权限控制
     * ========================================================= */
    static void setWidgetsByPermission(
        PermissionLevel minLevel,
        const QList<QWidget*>& widgets,
        const QList<PermissionLevel>& excludedLevels
        ) {
        for (QWidget* widget : widgets) {
            setUIByPermission(minLevel, widget, excludedLevels);
        }
    }

    /**
     * @brief 将权限字符串转换为枚举值
     *
     * 说明：
     * - SessionManager 中通常保存的是字符串形式
     * - 这里统一做映射，避免业务层出现字符串判断
     * - 未匹配到的情况默认返回 Standard（最低权限）
     */
    static int levelFromString(const QString& levelStr) {
        static QHash<QString, int> levelMap = {
            { "标准用户", Standard },
            { "Standard",  Standard },

            { "QA",        QA },

            { "主管",      Supervisor },
            { "Supervisor",Supervisor },

            { "管理员",    Admin },
            { "Admin",     Admin },

            { "超级管理员",SuperAdmin },
            { "SuperAdmin",SuperAdmin }
        };

        return levelMap.value(levelStr, Standard);
    }
};

#endif // PERMISSIONMANAGER_H
