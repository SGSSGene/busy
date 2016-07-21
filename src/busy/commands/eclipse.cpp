#include "commands.h"
#include <iostream>

using namespace aBuild;

namespace commands {

void generatePart1(std::ostream& ost, std::set<std::string> const& _includePaths, std::string const& _path);
void generatePart2(std::ostream& ost, std::set<std::string> const& _includePaths);

void generatePart3(std::ostream& ost, std::string const& _projectName);


void eclipse() {
	std::cout << "Generate .cproject and .project files" << std::endl;

	Package package {PackageURL()};
	serializer::yaml::read("busy.yaml", package);

	Workspace ws(".");


	std::set<std::string> includePaths;
//	auto const& allProjects = ws.getAllRequiredProjects();
	auto requiredProjects = ws.getAllRequiredProjects();

	auto packagesDir = utils::listDirs("extRepositories", true);
	for (auto& p : packagesDir) {
		p = "extRepositories/" + p;
	}
	packagesDir.push_back(".");

	// find auto dependencies
	std::vector<Project*> autoProjects;
	for (auto const& pDir : packagesDir) {
		if (not utils::fileExists(pDir + "/src")) continue;
		// Find auto projects
		auto projectDirs = utils::listDirs(pDir + "/src", true);
		for (auto const& d : projectDirs) {
			auto iter = requiredProjects.find(d);
			if (iter == requiredProjects.end()) {
				requiredProjects[d].set(d);
				requiredProjects[d].setPackagePath(pDir);
				requiredProjects[d].setAutoDependenciesDiscovery(true);
			}

			auto& project = requiredProjects.at(d);
			if (project.getIgnore()) continue;
			if (not project.getAutoDependenciesDiscovery()) continue;

			autoProjects.push_back(&project);
		}
	}
	for (auto p : autoProjects) {
		auto& project = *p;

		auto dep    = project.getDefaultDependencies(&ws, requiredProjects);
		auto optDep = project.getDefaultOptionalDependencies(&ws, requiredProjects);

		for (auto d : optDep) {
			auto iter = std::find(dep.begin(), dep.end(), d);
			while (iter != dep.end()) {
				dep.erase(iter);
				iter = std::find(dep.begin(), dep.end(), d);
			}
		}

		project.setDependencies(std::move(dep));
		project.setOptionalDependencies(std::move(optDep));
	}
	// save all the auto detected dependencies
	ws.save();


	// Create dependency tree
	//auto projects = requiredProjectso
	//	auto const& allProjects = ws.getAllRequiredProjects();
	//auto const& allProjects = autoProjects;
	std::map<std::string, Project> allProjects = requiredProjects;
/*	for (auto& p : autoProjects) {
		allProjects[p->getName()] = p;
	}*/



	for (auto p : allProjects) {
		for (auto d : p.second.getDependencies()) {
			auto parts = utils::explode(d, "/");
			std::string baseName = "";
			if (parts[0] != package.getName()) {
				baseName = "extRepositories/" + parts[0] + "/";
			}
			includePaths.insert(baseName + "src/" + parts[1]);

			for (auto const& i : allProjects.at(parts[1]).getLegacy().includes) {
				includePaths.insert(baseName + i);
			}
		}
	}

	for (auto p : includePaths) {
		std::cout<<p<<std::endl;
	}

	std::ofstream cproject(".cproject");
	generatePart1(cproject, includePaths, package.getName());

	std::ofstream project(".project");
	generatePart3(project, package.getName());

	std::cout << "Done" << std::endl;
}

void generatePart1(std::ostream& ost, std::set<std::string> const& _includePaths, std::string const& _projectName) {
	ost <<
R"|(<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<?fileVersion 4.0.0?><cproject storage_type_id="org.eclipse.cdt.core.XmlProjectDescriptionStorage">
	<storageModule moduleId="org.eclipse.cdt.core.settings">
		<cconfiguration id="cdt.managedbuild.config.gnu.exe.debug.496408544">
			<storageModule buildSystemId="org.eclipse.cdt.managedbuilder.core.configurationDataProvider" id="cdt.managedbuild.config.gnu.exe.debug.496408544" moduleId="org.eclipse.cdt.core.settings" name="Debug">
				<externalSettings/>
				<extensions>
					<extension id="org.eclipse.cdt.core.GmakeErrorParser" point="org.eclipse.cdt.core.ErrorParser"/>
					<extension id="org.eclipse.cdt.core.CWDLocator" point="org.eclipse.cdt.core.ErrorParser"/>
					<extension id="org.eclipse.cdt.core.GCCErrorParser" point="org.eclipse.cdt.core.ErrorParser"/>
					<extension id="org.eclipse.cdt.core.GASErrorParser" point="org.eclipse.cdt.core.ErrorParser"/>
					<extension id="org.eclipse.cdt.core.GLDErrorParser" point="org.eclipse.cdt.core.ErrorParser"/>
					<extension id="org.eclipse.cdt.core.ELF" point="org.eclipse.cdt.core.BinaryParser"/>
				</extensions>
			</storageModule>
			<storageModule moduleId="cdtBuildSystem" version="4.0.0">
				<configuration artifactName="${ProjName}" buildArtefactType="org.eclipse.cdt.build.core.buildArtefactType.exe" buildProperties="org.eclipse.cdt.build.core.buildType=org.eclipse.cdt.build.core.buildType.debug,org.eclipse.cdt.build.core.buildArtefactType=org.eclipse.cdt.build.core.buildArtefactType.exe" cleanCommand="rm -rf" description="" id="cdt.managedbuild.config.gnu.exe.debug.496408544" name="Debug" parent="cdt.managedbuild.config.gnu.exe.debug">
					<folderInfo id="cdt.managedbuild.config.gnu.exe.debug.496408544." name="/" resourcePath="">
						<toolChain id="cdt.managedbuild.toolchain.gnu.exe.debug.1670165173" name="Linux GCC" superClass="cdt.managedbuild.toolchain.gnu.exe.debug">
							<targetPlatform id="cdt.managedbuild.target.gnu.platform.exe.debug.833510060" name="Debug Platform" superClass="cdt.managedbuild.target.gnu.platform.exe.debug"/>
							<builder arguments="-c &quot;busy buildMode=debug ; busy --noterminal&quot;" buildPath="${workspace_loc:/)|" << _projectName << R"|(}/" command="bash" id="cdt.managedbuild.target.gnu.builder.exe.debug.986940758" keepEnvironmentInBuildfile="false" managedBuildOn="false" name="Gnu Make Builder" superClass="cdt.managedbuild.target.gnu.builder.exe.debug"/>
							<tool id="cdt.managedbuild.tool.gnu.archiver.base.1320209044" name="GCC Archiver" superClass="cdt.managedbuild.tool.gnu.archiver.base"/>
							<tool id="cdt.managedbuild.tool.gnu.cpp.compiler.exe.debug.1576934299" name="GCC C++ Compiler" superClass="cdt.managedbuild.tool.gnu.cpp.compiler.exe.debug">
								<option id="gnu.cpp.compiler.exe.debug.option.optimization.level.622376607" name="Optimization Level" superClass="gnu.cpp.compiler.exe.debug.option.optimization.level" value="gnu.cpp.compiler.optimization.level.none" valueType="enumerated"/>
								<option id="gnu.cpp.compiler.exe.debug.option.debugging.level.1663291674" name="Debug Level" superClass="gnu.cpp.compiler.exe.debug.option.debugging.level" value="gnu.cpp.compiler.debugging.level.max" valueType="enumerated"/>
								<option id="gnu.cpp.compiler.option.include.paths.569410827" superClass="gnu.cpp.compiler.option.include.paths" valueType="includePath">
)|";
	generatePart2(ost, _includePaths);
	ost <<
R"|(								</option>
								<inputType id="cdt.managedbuild.tool.gnu.cpp.compiler.input.1440063223" superClass="cdt.managedbuild.tool.gnu.cpp.compiler.input"/>
							</tool>
							<tool id="cdt.managedbuild.tool.gnu.c.compiler.exe.debug.961977755" name="GCC C Compiler" superClass="cdt.managedbuild.tool.gnu.c.compiler.exe.debug">
								<option defaultValue="gnu.c.optimization.level.none" id="gnu.c.compiler.exe.debug.option.optimization.level.662805321" name="Optimization Level" superClass="gnu.c.compiler.exe.debug.option.optimization.level" valueType="enumerated"/>
								<option id="gnu.c.compiler.exe.debug.option.debugging.level.555923421" name="Debug Level" superClass="gnu.c.compiler.exe.debug.option.debugging.level" value="gnu.c.debugging.level.max" valueType="enumerated"/>
								<option id="gnu.c.compiler.option.include.paths.1069828714" superClass="gnu.c.compiler.option.include.paths" valueType="includePath">
)|";
	generatePart2(ost, _includePaths);
	ost <<
R"|(								</option>
								<inputType id="cdt.managedbuild.tool.gnu.c.compiler.input.1807724777" superClass="cdt.managedbuild.tool.gnu.c.compiler.input"/>
							</tool>
							<tool id="cdt.managedbuild.tool.gnu.c.linker.exe.debug.1727285297" name="GCC C Linker" superClass="cdt.managedbuild.tool.gnu.c.linker.exe.debug"/>
							<tool id="cdt.managedbuild.tool.gnu.cpp.linker.exe.debug.760564045" name="GCC C++ Linker" superClass="cdt.managedbuild.tool.gnu.cpp.linker.exe.debug">
								<inputType id="cdt.managedbuild.tool.gnu.cpp.linker.input.1022001277" superClass="cdt.managedbuild.tool.gnu.cpp.linker.input">
									<additionalInput kind="additionalinputdependency" paths="$(USER_OBJS)"/>
									<additionalInput kind="additionalinput" paths="$(LIBS)"/>
								</inputType>
							</tool>
							<tool id="cdt.managedbuild.tool.gnu.assembler.exe.debug.976162968" name="GCC Assembler" superClass="cdt.managedbuild.tool.gnu.assembler.exe.debug">
								<option id="gnu.both.asm.option.include.paths.1280156146" superClass="gnu.both.asm.option.include.paths" valueType="includePath">
)|";
	generatePart2(ost, _includePaths);
	ost <<
R"|(								</option>
								<inputType id="cdt.managedbuild.tool.gnu.assembler.input.1800382991" superClass="cdt.managedbuild.tool.gnu.assembler.input"/>
							</tool>
						</toolChain>
					</folderInfo>
				</configuration>
			</storageModule>
			<storageModule moduleId="org.eclipse.cdt.core.externalSettings"/>
		</cconfiguration>
		<cconfiguration id="cdt.managedbuild.config.gnu.exe.release.223859876">
			<storageModule buildSystemId="org.eclipse.cdt.managedbuilder.core.configurationDataProvider" id="cdt.managedbuild.config.gnu.exe.release.223859876" moduleId="org.eclipse.cdt.core.settings" name="Release">
				<externalSettings/>
				<extensions>
					<extension id="org.eclipse.cdt.core.GmakeErrorParser" point="org.eclipse.cdt.core.ErrorParser"/>
					<extension id="org.eclipse.cdt.core.CWDLocator" point="org.eclipse.cdt.core.ErrorParser"/>
					<extension id="org.eclipse.cdt.core.GCCErrorParser" point="org.eclipse.cdt.core.ErrorParser"/>
					<extension id="org.eclipse.cdt.core.GASErrorParser" point="org.eclipse.cdt.core.ErrorParser"/>
					<extension id="org.eclipse.cdt.core.GLDErrorParser" point="org.eclipse.cdt.core.ErrorParser"/>
					<extension id="org.eclipse.cdt.core.ELF" point="org.eclipse.cdt.core.BinaryParser"/>
				</extensions>
			</storageModule>
			<storageModule moduleId="cdtBuildSystem" version="4.0.0">
				<configuration artifactName="${ProjName}" buildArtefactType="org.eclipse.cdt.build.core.buildArtefactType.exe" buildProperties="org.eclipse.cdt.build.core.buildType=org.eclipse.cdt.build.core.buildType.release,org.eclipse.cdt.build.core.buildArtefactType=org.eclipse.cdt.build.core.buildArtefactType.exe" cleanCommand="rm -rf" description="" id="cdt.managedbuild.config.gnu.exe.release.223859876" name="Release" parent="cdt.managedbuild.config.gnu.exe.release">
					<folderInfo id="cdt.managedbuild.config.gnu.exe.release.223859876." name="/" resourcePath="">
						<toolChain id="cdt.managedbuild.toolchain.gnu.exe.release.1083116967" name="Linux GCC" superClass="cdt.managedbuild.toolchain.gnu.exe.release">
							<targetPlatform id="cdt.managedbuild.target.gnu.platform.exe.release.1361131952" name="Debug Platform" superClass="cdt.managedbuild.target.gnu.platform.exe.release"/>
							<builder arguments="-c &quot;busy buildMode=release ; busy --noterminal&quot;" buildPath="${workspace_loc:/)|" << _projectName << R"|(}/" command="bash" id="cdt.managedbuild.target.gnu.builder.exe.release.1915712032" keepEnvironmentInBuildfile="false" managedBuildOn="false" name="Gnu Make Builder" superClass="cdt.managedbuild.target.gnu.builder.exe.release"/>
							<tool id="cdt.managedbuild.tool.gnu.archiver.base.524627035" name="GCC Archiver" superClass="cdt.managedbuild.tool.gnu.archiver.base"/>
							<tool id="cdt.managedbuild.tool.gnu.cpp.compiler.exe.release.1687446195" name="GCC C++ Compiler" superClass="cdt.managedbuild.tool.gnu.cpp.compiler.exe.release">
								<option id="gnu.cpp.compiler.exe.release.option.optimization.level.69011137" name="Optimization Level" superClass="gnu.cpp.compiler.exe.release.option.optimization.level" value="gnu.cpp.compiler.optimization.level.most" valueType="enumerated"/>
								<option id="gnu.cpp.compiler.exe.release.option.debugging.level.2111836686" name="Debug Level" superClass="gnu.cpp.compiler.exe.release.option.debugging.level" value="gnu.cpp.compiler.debugging.level.none" valueType="enumerated"/>
								<option id="gnu.cpp.compiler.option.include.paths.87126948" superClass="gnu.cpp.compiler.option.include.paths" valueType="includePath">"|);
)|";
	generatePart2(ost, _includePaths);
	ost <<
R"|(								</option>
								<inputType id="cdt.managedbuild.tool.gnu.cpp.compiler.input.1206870041" superClass="cdt.managedbuild.tool.gnu.cpp.compiler.input"/>
							</tool>
							<tool id="cdt.managedbuild.tool.gnu.c.compiler.exe.release.1306416481" name="GCC C Compiler" superClass="cdt.managedbuild.tool.gnu.c.compiler.exe.release">
								<option defaultValue="gnu.c.optimization.level.most" id="gnu.c.compiler.exe.release.option.optimization.level.1122545453" name="Optimization Level" superClass="gnu.c.compiler.exe.release.option.optimization.level" valueType="enumerated"/>
								<option id="gnu.c.compiler.exe.release.option.debugging.level.690637233" name="Debug Level" superClass="gnu.c.compiler.exe.release.option.debugging.level" value="gnu.c.debugging.level.none" valueType="enumerated"/>
								<option id="gnu.c.compiler.option.include.paths.1413440810" superClass="gnu.c.compiler.option.include.paths" valueType="includePath">
)|";
	generatePart2(ost, _includePaths);
	ost <<
R"|(								</option>
								<inputType id="cdt.managedbuild.tool.gnu.c.compiler.input.453408675" superClass="cdt.managedbuild.tool.gnu.c.compiler.input"/>
							</tool>
							<tool id="cdt.managedbuild.tool.gnu.c.linker.exe.release.1425142623" name="GCC C Linker" superClass="cdt.managedbuild.tool.gnu.c.linker.exe.release"/>
							<tool id="cdt.managedbuild.tool.gnu.cpp.linker.exe.release.1900710104" name="GCC C++ Linker" superClass="cdt.managedbuild.tool.gnu.cpp.linker.exe.release">
								<inputType id="cdt.managedbuild.tool.gnu.cpp.linker.input.1825473251" superClass="cdt.managedbuild.tool.gnu.cpp.linker.input">
									<additionalInput kind="additionalinputdependency" paths="$(USER_OBJS)"/>
									<additionalInput kind="additionalinput" paths="$(LIBS)"/>
								</inputType>
							</tool>
							<tool id="cdt.managedbuild.tool.gnu.assembler.exe.release.1813252093" name="GCC Assembler" superClass="cdt.managedbuild.tool.gnu.assembler.exe.release">
								<option id="gnu.both.asm.option.include.paths.779843760" superClass="gnu.both.asm.option.include.paths" valueType="includePath">
)|";
	generatePart2(ost, _includePaths);
	ost <<
R"|(								</option>
								<inputType id="cdt.managedbuild.tool.gnu.assembler.input.1533865849" superClass="cdt.managedbuild.tool.gnu.assembler.input"/>
							</tool>
						</toolChain>
					</folderInfo>
				</configuration>
			</storageModule>
			<storageModule moduleId="org.eclipse.cdt.core.externalSettings"/>
		</cconfiguration>
	</storageModule>
	<storageModule moduleId="cdtBuildSystem" version="4.0.0">
		<project id=")|" << _projectName << R"|(.cdt.managedbuild.target.gnu.exe.1735529810" name="Executable" projectType="cdt.managedbuild.target.gnu.exe"/>
	</storageModule>
	<storageModule moduleId="scannerConfiguration">
		<autodiscovery enabled="true" problemReportingEnabled="true" selectedProfileId=""/>
		<scannerConfigBuildInfo instanceId="cdt.managedbuild.config.gnu.exe.release.223859876;cdt.managedbuild.config.gnu.exe.release.223859876.;cdt.managedbuild.tool.gnu.cpp.compiler.exe.release.1687446195;cdt.managedbuild.tool.gnu.cpp.compiler.input.1206870041">
			<autodiscovery enabled="true" problemReportingEnabled="true" selectedProfileId=""/>
		</scannerConfigBuildInfo>
		<scannerConfigBuildInfo instanceId="cdt.managedbuild.config.gnu.exe.debug.496408544;cdt.managedbuild.config.gnu.exe.debug.496408544.;cdt.managedbuild.tool.gnu.c.compiler.exe.debug.961977755;cdt.managedbuild.tool.gnu.c.compiler.input.1807724777">
			<autodiscovery enabled="true" problemReportingEnabled="true" selectedProfileId=""/>
		</scannerConfigBuildInfo>
		<scannerConfigBuildInfo instanceId="cdt.managedbuild.config.gnu.exe.release.223859876;cdt.managedbuild.config.gnu.exe.release.223859876.;cdt.managedbuild.tool.gnu.c.compiler.exe.release.1306416481;cdt.managedbuild.tool.gnu.c.compiler.input.453408675">
			<autodiscovery enabled="true" problemReportingEnabled="true" selectedProfileId=""/>
		</scannerConfigBuildInfo>
		<scannerConfigBuildInfo instanceId="cdt.managedbuild.config.gnu.exe.debug.496408544;cdt.managedbuild.config.gnu.exe.debug.496408544.;cdt.managedbuild.tool.gnu.cpp.compiler.exe.debug.1576934299;cdt.managedbuild.tool.gnu.cpp.compiler.input.1440063223">
			<autodiscovery enabled="true" problemReportingEnabled="true" selectedProfileId=""/>
		</scannerConfigBuildInfo>
	</storageModule>
	<storageModule moduleId="org.eclipse.cdt.core.LanguageSettingsProviders"/>
	<storageModule moduleId="refreshScope" versionNumber="2">
		<configuration configurationName="Release">
)|";
ost <<
R"|(			<resource resourceType="PROJECT" workspacePath="/)|" << _projectName << R"|("/>
)|";

	ost <<
R"|(		</configuration>
		<configuration configurationName="Debug">
)|";
ost <<
R"|(			<resource resourceType="PROJECT" workspacePath="/)|" << _projectName << R"|("/>
)|";
	ost <<
R"|(
		</configuration>
	</storageModule>
</cproject>
)|";

}

void generatePart2(std::ostream& ost, std::set<std::string> const& _includePaths) {
	for (auto s : _includePaths) {
		ost << 
R"|(									<listOptionValue builtIn="false" value="&quot;${workspace_loc:/)|" << s << R"|(}&quot;"/>
)|";
	}

}

void generatePart3(std::ostream& ost, std::string const& _projectName) {
ost <<
R"|(<?xml version="1.0" encoding="UTF-8"?>
<projectDescription>
	<name>)|" << _projectName << R"|(</name>
	<comment></comment>
	<projects>
	</projects>
	<buildSpec>
		<buildCommand>
			<name>org.eclipse.cdt.managedbuilder.core.genmakebuilder</name>
			<triggers>clean,full,incremental,</triggers>
			<arguments>
			</arguments>
		</buildCommand>
		<buildCommand>
			<name>org.eclipse.cdt.managedbuilder.core.ScannerConfigBuilder</name>
			<triggers>full,incremental,</triggers>
			<arguments>
			</arguments>
		</buildCommand>
	</buildSpec>
	<natures>
		<nature>org.eclipse.cdt.core.cnature</nature>
		<nature>org.eclipse.cdt.core.ccnature</nature>
		<nature>org.eclipse.cdt.managedbuilder.core.managedBuildNature</nature>
		<nature>org.eclipse.cdt.managedbuilder.core.ScannerConfigNature</nature>
	</natures>
</projectDescription>
)|";
}





}

