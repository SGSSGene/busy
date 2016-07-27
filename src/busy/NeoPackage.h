#pragma once

#include "NeoProject.h"
#include "NeoFlavor.h"
#include "NeoToolchain.h"

#include <list>
#include <string>

namespace busy {
	class NeoWorkspace;

	struct NeoPackageURL {
		std::string name;
		std::string url;
		std::string branch;
	};

	class NeoPackage {
	private:
		NeoWorkspace* mWorkspace;

		std::string   mName;
		std::string   mPath;

		std::list<NeoProject> mProjects;
		std::vector<NeoPackageURL> mExternalRepURLs;
		std::vector<NeoPackage*>   mExternalPackages;

		std::map<std::string, NeoFlavor>    mFlavors;
		std::map<std::string, NeoToolchain> mToolchains;

	public:

		NeoPackage(std::string const& _path, NeoWorkspace* _workspace);

		void setupPackageDependencies();

		auto getWorkspace() const -> NeoWorkspace* { return mWorkspace; }



		auto getName() const -> std::string const& { return mName; }
		auto getPath() const -> std::string const& { return mPath; }
		auto getProjects() const -> std::list<NeoProject> const& { return mProjects; }
		auto getProjects()       -> std::list<NeoProject>&       { return mProjects; }

		auto getExternalPackageURLs() -> std::vector<NeoPackageURL> const& { return mExternalRepURLs; }
		auto getExternalPackages() -> std::vector<NeoPackage*> const& { return mExternalPackages; }
		auto getAllDependendPackages() -> std::vector<NeoPackage*>;

		auto getFlavors() const -> std::map<std::string, NeoFlavor> const& { return mFlavors; }
		auto getToolchains() const -> std::map<std::string, NeoToolchain> const& { return mToolchains; }

		auto getProject(std::string const& _name) const -> NeoProject const&;
	};
}
