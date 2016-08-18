#pragma once

#include "Flavor.h"
#include "Override.h"
#include "Project.h"
#include "Toolchain.h"

#include <list>
#include <string>

namespace busy {
	class Workspace;

	struct PackageURL {
		std::string name;
		std::string url;
		std::string branch;
	};

	class Package {
	private:
		Workspace* mWorkspace;

		std::string   mName;
		std::string   mPath;

		std::list<Project> mProjects;
		std::vector<PackageURL> mExternalRepURLs;
		std::vector<Package*>   mExternalPackages;

		std::map<std::string, Flavor>    mFlavors;
		std::map<std::string, Toolchain> mToolchains;
		Overrides                        mOverrides;

	public:

		Package(std::string const& _path, Workspace* _workspace);

		void setupPackageDependencies();

		auto getWorkspace() const -> Workspace* { return mWorkspace; }



		auto getName() const -> std::string const& { return mName; }
		auto getPath() const -> std::string const& { return mPath; }
		auto getProjects() const -> std::list<Project> const& { return mProjects; }
		auto getProjects()       -> std::list<Project>&       { return mProjects; }

		auto getExternalPackageURLs() -> std::vector<PackageURL> const& { return mExternalRepURLs; }
		auto getExternalPackages() -> std::vector<Package*> const& { return mExternalPackages; }
		auto getAllDependendPackages() -> std::vector<Package*>;

		auto getFlavors() const -> std::map<std::string, Flavor> const& { return mFlavors; }
		auto getToolchains() const -> std::map<std::string, Toolchain> const& { return mToolchains; }

		bool hasProject(std::string const& _name) const;
		auto getProject(std::string const& _name) const -> Project const&;
		auto getOverrides() const -> Overrides const& { return mOverrides; }
	};
}
