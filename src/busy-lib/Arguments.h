#pragma once

#include <clice/clice.h>


inline auto cliModeCompile = clice::Argument{ .arg    = {"compile"},
                                              .desc   = {"compile everything"}
                                            };
inline auto cliModeStatus  = clice::Argument{ .arg    = {"status"},
                                              .desc   = {"current status of compilation"},
                                            };
inline auto cliModeInfo    = clice::Argument{ .arg    = {"info"},
                                              .desc   = {"print some infos about available packages"}
                                            };
inline auto cliModeInstall = clice::Argument{ .arg    = {"install"},
                                              .desc   = {"install binaries to machine"}
                                            };
inline auto cliFile        = clice::Argument{ .arg    = {"-f"},
                                              .desc   = "path to a busy.yaml file",
                                              .value  = std::filesystem::path{},
                                            };
inline auto cliBuildPath   = clice::Argument{ .arg    = {"-C"},
                                              .desc   = "path to the build path",
                                              .value  = std::filesystem::path{"."},
                                            };
inline auto cliToolchains  = clice::Argument{ .arg    = {"-t"},
                                              .desc   = "set a toolchain",
                                              .value  = std::vector<std::string>{},
                                            };
inline auto cliJobs        = clice::Argument{ .arg    = {"-j"},
                                              .desc   = "set the number of threads",
                                              .value  = size_t{1},
                                            };
inline auto cliOptions     = clice::Argument{ .arg    = {"--options"},
                                              .desc   = "options given to the toolchains",
                                              .value  = std::vector<std::string>{"debug"},
                                            };
inline auto cliClean       = clice::Argument{ .arg    = {"--clean"},
                                              .desc   = "force a rebuild",
                                            };
inline auto cliVerbose     = clice::Argument{ .arg    = {"--verbose"},
                                              .desc   = "verbose run",
                                            };
inline auto cliPrefix      = clice::Argument{ .parent = &cliModeInstall,
                                              .arg    = {"--prefix"},
                                              .desc   = "prefix for installation",
                                              .value = std::filesystem::path{},
                                            };
