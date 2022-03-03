#include "analyse.h"


namespace busy {

// check if this _project is included by _allIncludes
auto isDependentTranslationSet(std::set<std::filesystem::path> const& _allIncludes, TranslationSet const& _project) -> bool {

    auto files = _project.getFiles();
    for (auto const& file : files) {
        // check if is includable by default path
        {
            auto path = _project.getName() / relative(file.getPath(), _project.getPath());
            if (_allIncludes.count(path)) {
                return true;
            }
        }
        // check if it is includable by legacy include path
        {
            for (auto const& p : _project.getLegacyIncludePaths()) {
                auto path = relative(file.getPath(), p);
                if (_allIncludes.count(path)) {
                    return true;
                }
            }
        }
    }
    return false;
}

auto findDependentTranslationSets(TranslationSet const& _project, std::vector<TranslationSet> const& _projects) -> std::set<TranslationSet const*> {
    auto ret = std::set<TranslationSet const*>{};
    auto _allIncludes = _project.getIncludes();

    for (auto const& project : _projects) {
        if (isDependentTranslationSet(_allIncludes, project)) {
            ret.emplace(&project);
        }
    }
    return ret;
}

auto createTranslationSets(std::vector<TranslationSet> const& _projects) -> TranslationSetMap {
    auto ret = TranslationSetMap{};

    for (auto const& p : _projects) {
        ret[&p];
        auto deps = findDependentTranslationSets(p, _projects);
        for (auto const& d : deps) {
            if (d == &p) continue;
            std::get<0>(ret[&p]).insert(d);
            std::get<1>(ret[d]).insert(&p);
        }
    }

    return normalizeTranslationSets(ret);
}
auto normalizeTranslationSets(TranslationSetMap const& _projectMap) -> TranslationSetMap {
    auto duplicateList = std::map<std::string, TranslationSet const*>{};
    auto ret = TranslationSetMap{};
    for (auto [key, deps] : _projectMap) {
        auto iter = duplicateList.find(key->getName());
        if (iter == end(duplicateList)) {
            duplicateList[key->getName()] = key;
            ret[key] = deps;
        }
    }
    for (auto& [key, deps] : ret) {
        auto ingoing  = std::get<0>(deps);
        auto outgoing = std::get<1>(deps);
        deps = {};

        for (auto v : ingoing) {
            std::get<0>(deps).insert(duplicateList.at(v->getName()));
        }
        for (auto v : outgoing) {
            std::get<1>(deps).insert(duplicateList.at(v->getName()));
        }
    }
    return ret;
}


void checkConsistency(std::vector<TranslationSet> const& _projects) {
    auto groupedTranslationSets = std::map<std::string, std::vector<TranslationSet const*>>{};
    for (auto const& p : _projects) {
        groupedTranslationSets[p.getName()].emplace_back(&p);
    }


    for (auto const& [name, list] : groupedTranslationSets) {
        //!TODO this can be done much more efficient
        if (size(list) > 1) {
            for (auto const& p1 : list) {
                for (auto const& p2 : list) {
                    if (p1 == p2) continue;
                    if (not p1->isEquivalent(*p2)) {
                        throw std::runtime_error("two projects don't seem to be the same: " + p1->getName());
                    }
                }
            }
        }
    }
}

auto listConsistencyIssues(std::vector<TranslationSet> const& _projects) -> std::vector<std::tuple<TranslationSet const*, TranslationSet const*>> {
    auto groupedTranslationSets = std::map<std::string, std::vector<TranslationSet const*>>{};
    for (auto const& p : _projects) {
        groupedTranslationSets[p.getName()].emplace_back(&p);
    }

    auto retList = std::vector<std::tuple<TranslationSet const*, TranslationSet const*>>{};
    for (auto const& [name, list] : groupedTranslationSets) {
        if (size(list) > 1) {
            for (auto const& p1 : list) {
                for (auto const& p2 : list) {
                    if (p1 == p2) continue;
                    if (not p1->isEquivalent(*p2)) {
                        retList.emplace_back(p1, p2);
                    }
                }
            }
        }
    }
    return retList;
}

}
