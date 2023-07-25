/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once

#include <libyang-cpp/export.h>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

struct ly_ctx;
struct lys_module;
struct lysc_ident;
struct lysc_ext;
struct lysc_ext_instance;
struct lysp_feature;

namespace libyang {
class Context;
class DataNode;
class DataNodeTerm;
class Extension;
class ExtensionInstance;
class Meta;
class Module;
class ChildInstanstiables;
class Identity;
class SchemaNode;

namespace types {
class IdentityRef;
}

/**
 * @brief Represents a feature of a module.
 *
 * Wraps `lysp_feature`.
 */
class LIBYANG_CPP_EXPORT Feature {
public:
    std::string_view name() const;

    friend Module;

private:
    Feature(const lysp_feature* feature, std::shared_ptr<ly_ctx> ctx);

    const lysp_feature* m_feature;
    std::shared_ptr<ly_ctx> m_ctx;
};

/**
 * @brief Tag for enabling all features (as if using "*" from libyang).
 *
 * See Module::setImplemented for more information.
 */

struct LIBYANG_CPP_EXPORT AllFeatures {
};

/**
 * @brief libyang module class.
 */
class LIBYANG_CPP_EXPORT Module {
public:
    std::string_view name() const;
    std::optional<std::string_view> revision() const;
    bool implemented() const;
    bool featureEnabled(const std::string& featureName) const;
    std::vector<Feature> features() const;
    std::vector<ExtensionInstance> extensions() const;
    void setImplemented();
    void setImplemented(std::vector<std::string> features);
    void setImplemented(const AllFeatures);

    std::vector<Identity> identities() const;

    ChildInstanstiables childInstantiables() const;

    friend Context;
    friend DataNode;
    friend Meta;
    friend Identity;
    friend SchemaNode;

private:
    Module(lys_module* module, std::shared_ptr<ly_ctx> ctx);

    std::shared_ptr<ly_ctx> m_ctx;
    lys_module* m_module;
};

/**
 * @brief Contains information about an identity.
 *
 * Wraps `lysc_ident`.
 */
class LIBYANG_CPP_EXPORT Identity {
public:
    friend DataNodeTerm;
    friend Module;
    friend types::IdentityRef;
    std::vector<Identity> derived() const;
    std::vector<Identity> derivedRecursive() const;
    Module module() const;
    std::string_view name() const;

    bool operator==(const Identity& other) const;

private:
    Identity(const lysc_ident* ident, std::shared_ptr<ly_ctx> ctx);

    const lysc_ident* m_ident;
    std::shared_ptr<ly_ctx> m_ctx;
};

/**
 * @brief Contains information about compiled extension.
 *
 * Wraps `lysc_extension_instance`
 */
class LIBYANG_CPP_EXPORT ExtensionInstance {
public:
    Extension definition() const;
    std::string_view argument() const;

private:
    ExtensionInstance(const lysc_ext_instance* ext, std::shared_ptr<ly_ctx> ctx);

    const lysc_ext_instance* m_ext;
    std::shared_ptr<ly_ctx> m_ctx;

    friend Module;
    friend Context;
};

/**
 * @brief Contains information about an extension definition.
 *
 * Wraps `lysc_ext`
 */
class LIBYANG_CPP_EXPORT Extension {
public:
    std::string_view name() const;

private:
    Extension(const lysc_ext* def, std::shared_ptr<ly_ctx> ctx);

    const lysc_ext* m_ext;
    std::shared_ptr<ly_ctx> m_ctx;

    friend ExtensionInstance;
};
}
