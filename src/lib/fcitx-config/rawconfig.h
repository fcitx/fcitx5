/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_CONFIG_RAWCONFIG_H_
#define _FCITX_CONFIG_RAWCONFIG_H_

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <fcitx-config/fcitxconfig_export.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/macros.h>

namespace fcitx {

class RawConfig;

/** Shared pointer type for RawConfig. */
using RawConfigPtr = std::shared_ptr<RawConfig>;

class RawConfigPrivate;
/**
 * A raw configuration tree that stores key-value pairs in a hierarchical
 * structure.
 *
 * RawConfig represents a configuration as a tree of nodes, where each node
 * can have a value and sub-items. For example, It supports reading from and
 * writing to INI format, and can represent configuration with implicit
 * (default) values.
 */
class FCITXCONFIG_EXPORT RawConfig {
public:
    /** Construct a RawConfig with an empty name. */
    RawConfig();
    FCITX_DECLARE_VIRTUAL_DTOR_COPY(RawConfig)

    /**
     * Get a sub-item by path.
     *
     * @param path path to the sub-item, using '/' as separator
     * @param create if true, create intermediate nodes if they don't exist
     * @return shared pointer to the sub-item, or nullptr if not found and
     *         create is false
     */
    std::shared_ptr<RawConfig> get(const std::string &path,
                                   bool create = false);
    /** @overload */
    std::shared_ptr<const RawConfig> get(const std::string &path) const;

    /**
     * Remove a sub-item by path.
     *
     * @param path path to the item to remove
     * @return true if the item was removed, false if not found or invalid path
     */
    bool remove(const std::string &path);

    /** Remove all sub-items. */
    void removeAll();

    /**
     * Set the value of this node.
     *
     * @param value the value to set
     */
    void setValue(std::string value);

    /**
     * Set the comment for this node.
     *
     * @param comment the comment to set
     */
    void setComment(std::string comment);

    /**
     * Set the line number where this node was parsed from.
     *
     * @param lineNumber the line number
     */
    void setLineNumber(unsigned int lineNumber);

    /**
     * Mark this config node as implicit (default value that won't be written
     * to INI).
     *
     * @param implicit true to mark as implicit, false otherwise
     *
     * @since 5.1.22
     */
    void setImplicit(bool implicit);

    /** Get the name of this node. */
    const std::string &name() const;

    /** Get the comment for this node. */
    const std::string &comment() const;

    /** Get the value of this node. */
    const std::string &value() const;

    /** Get the line number where this node was parsed from. */
    unsigned int lineNumber() const;

    /**
     * Check if this node itself is marked as implicit.
     *
     * @return true if this node is marked as implicit
     *
     * @since 5.1.22
     */
    bool isImplicitSelf() const;

    /**
     * Check if this node or any of its ancestors is marked as implicit.
     *
     * @return true if this node or any ancestor is marked as implicit
     *
     * @since 5.1.22
     */
    bool isImplicit() const;

    /**
     * Check if this node has any sub-items.
     *
     * @return true if this node has sub-items
     */
    bool hasSubItems() const;

    /**
     * Get the number of sub-items.
     *
     * @return number of sub-items
     */
    size_t subItemsSize() const;

    /**
     * Get a list of sub-item names.
     *
     * @return list of sub-item names
     */
    std::vector<std::string> subItems() const;

    /**
     * Set a value by path, creating intermediate nodes if needed.
     *
     * @param path path to the value, using '/' as separator
     * @param value the value to set
     */
    void setValueByPath(const std::string &path, std::string value) {
        (*this)[path] = std::move(value);
    }

    /**
     * Get a value by path.
     *
     * @param path path to the value, using '/' as separator
     * @return pointer to the value, or nullptr if not found
     */
    const std::string *valueByPath(const std::string &path) const {
        auto config = get(path);
        return config ? &config->value() : nullptr;
    }

    /**
     * Get or create a sub-item by path.
     *
     * @param path path to the sub-item, using '/' as separator
     * @return reference to the sub-item
     */
    RawConfig &operator[](const std::string &path) { return *get(path, true); }

    /**
     * Assign a string value to this node.
     *
     * @param value the value to assign
     * @return reference to this node
     */
    RawConfig &operator=(std::string value) {
        setValue(std::move(value));
        return *this;
    }

    /**
     * Check if two RawConfig trees are equal.
     *
     * @param other the other RawConfig to compare with
     * @return true if the trees are equal
     */
    bool operator==(const RawConfig &other) const {
        if (this == &other) {
            return true;
        }
        if (value() != other.value()) {
            return false;
        }
        if (subItemsSize() != other.subItemsSize()) {
            return false;
        }
        return visitSubItems(
            [&other](const RawConfig &subConfig, const std::string &path) {
                auto otherSubConfig = other.get(path);
                return (otherSubConfig && *otherSubConfig == subConfig);
            });
    }

    /**
     * Check if two RawConfig trees are not equal.
     *
     * @param config the other RawConfig to compare with
     * @return true if the trees are not equal
     */
    bool operator!=(const RawConfig &config) const {
        return !(*this == config);
    }

    /**
     * Get the parent node.
     *
     * @return pointer to the parent node, or nullptr if this is the root
     */
    RawConfig *parent() const;

    /**
     * Detach this node from its parent.
     *
     * @return shared pointer to the detached node, or empty if already root
     */
    std::shared_ptr<RawConfig> detach();

    /**
     * Visit all sub-items.
     *
     * @param callback function to call for each sub-item, return false to stop
     * @param path path to start visiting from, empty for root
     * @param recursive if true, visit sub-items recursively
     * @param pathPrefix prefix to prepend to paths in callback
     * @return true if all items were visited, false if stopped early
     */
    bool visitSubItems(
        std::function<bool(RawConfig &, const std::string &path)> callback,
        const std::string &path = "", bool recursive = false,
        const std::string &pathPrefix = "");
    /** @overload */
    bool visitSubItems(
        std::function<bool(const RawConfig &, const std::string &path)>
            callback,
        const std::string &path = "", bool recursive = false,
        const std::string &pathPrefix = "") const;

    /**
     * Visit all items on a path.
     *
     * @param callback function to call for each item on the path
     * @param path the path to visit
     */
    void visitItemsOnPath(
        std::function<void(RawConfig &, const std::string &path)> callback,
        const std::string &path);

    /** @overload */
    void visitItemsOnPath(
        std::function<void(const RawConfig &, const std::string &path)>
            callback,
        const std::string &path) const;

private:
    friend class RawConfigPrivate;
    RawConfig(std::string name);
    std::shared_ptr<RawConfig> createSub(std::string name);
    FCITX_DECLARE_PRIVATE(RawConfig);
    std::unique_ptr<RawConfigPrivate> d_ptr;
};

/**
 * Output a RawConfig to a log message.
 *
 * @param log the log message builder
 * @param config the RawConfig to output
 * @return reference to the log message builder
 */
FCITXCONFIG_EXPORT LogMessageBuilder &operator<<(LogMessageBuilder &log,
                                                 const RawConfig &config);
} // namespace fcitx

#endif // _FCITX_CONFIG_RAWCONFIG_H_
