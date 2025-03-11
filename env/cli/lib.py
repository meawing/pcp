import os
import subprocess
import sys
import typing as tp
import yaml
import click
import xml.etree.ElementTree as ET

from pathlib import Path
from termcolor import cprint


################################################################################


SEP_SIZE = 79
COURSE_DIRECTORY = Path(os.environ["COURSE_DIRECTORY"])
SYSTEM = os.environ["system"]
ASAN_SYMBOLIZER_PATH = os.environ["ASAN_SYMBOLIZER_PATH"]
TSAN_SYMBOLIZER_PATH = os.environ["TSAN_SYMBOLIZER_PATH"]


################################################################################


class OrderCommands(click.Group):
    def list_commands(self, ctx: click.Context) -> list[str]:
        return list(self.commands)


################################################################################


def get_build_directory(profile: str):
    return COURSE_DIRECTORY / "build" / profile


def cprinte(*args, **kwargs):
    cprint(*args, **kwargs, file=sys.stderr)


def print_sep(color: str = None):
    cprinte("=" * SEP_SIZE, color, attrs=["bold"])


def print_start(text: str):
    print_sep("yellow")
    cprinte(text, "yellow", attrs=["bold"])
    print_sep("yellow")


def print_fail(text: str):
    print_sep("red")
    cprinte(text, "red", attrs=["bold"])
    print_sep("red")
    cprinte("")


def print_success(text: str):
    print_sep("green")
    cprinte(text, "green", attrs=["bold"])
    print_sep("green")
    cprinte("")


def to_upper_case(profile: str):
    assert profile.lower() == profile and \
        " " not in profile and \
        "_" not in profile, "Invalid profile"
    return profile.replace("-", "_").upper()


################################################################################


def configure(profile: str):
    build_directory = get_build_directory(profile)
    cprinte(f"Configuring profile {profile} in build directory {build_directory}",
            "cyan")

    subprocess.run([
        "cmake",
        "-S", COURSE_DIRECTORY,
        "-B", build_directory,
        f"-DCMAKE_BUILD_TYPE={to_upper_case(profile)}",
        "-GNinja"
    ]).check_returncode()


def build_executable(target: str, profile: str):
    configure(profile)

    build_directory = get_build_directory(profile)

    cprinte(f"Building target {target}.{profile} in build directory {build_directory}",
            "cyan")

    subprocess.run([
        "cmake",
        "--build",
        build_directory,
        "--target",
        target
    ]).check_returncode()

    if SYSTEM == "x86_64-darwin":
        # It is necessary to generate .dSYM directory for symbolizers to work correctly.
        subprocess.run([
            "dsymutil",
            build_directory / target,
        ]).check_returncode()


def run_test(target: str, profile: str, sandbox: bool, filter: str | None = None) -> bool:
    print_start(f"Running test {target}.{profile}")

    try:
        build_executable(target, profile)

        build_directory = get_build_directory(profile)

        cprinte(f"Running test {target}.{profile}", "cyan")

        if not sandbox:
            subprocess.run([
                build_directory / target
            ] + ([filter] if filter else [])).check_returncode()
        else:
            subprocess.run([
                "bwrap",
                "--ro-bind",
                "/nix",
                "/nix",
                "--ro-bind",
                "/proc",
                "/proc",
                "--ro-bind",
                build_directory / target,
                target,
                "--clearenv",
                "--setenv",
                "TSAN_SYMBOLIZER_PATH",
                TSAN_SYMBOLIZER_PATH,
                "--setenv",
                "ASAN_SYMBOLIZER_PATH",
                ASAN_SYMBOLIZER_PATH,
                f"./{target}",
            ] + ([filter] if filter else [])).check_returncode()
    except subprocess.CalledProcessError as error:
        cprinte(str(error))
        print_fail(f"Test {target}.{profile} failed")
        return False
    else:
        print_success(f"Test {target}.{profile} succeded")
        return True


def run_linter(profile: str, files: tp.List[str]) -> bool:
    print_start(f"Running lint check with profile {profile} for {len(files)} files")

    try:
        configure(profile)
        subprocess.run(
            ["clang-tidy", "-p", get_build_directory(profile),
             "--config-file", COURSE_DIRECTORY / ".clang-tidy"] + files
        ).check_returncode()
    except subprocess.CalledProcessError:
        print_fail(f"Lint check with profile {profile} failed")
        return False
    else:
        print_success(f"Lint check with profile {profile} succeded")
        return True


def run_format(files: tp.List[str]) -> bool:
    print_start(f"Running format check for {len(files)} files")

    try:
        subprocess.run(
            ["clang-format", "--dry-run", "-Werror",
             f"--style=file:{COURSE_DIRECTORY / '.clang-format'}"] + files
        ).check_returncode()
    except subprocess.CalledProcessError:
        print_fail("Format check failed")
        return False
    else:
        print_success("Format check succeded")
        return True


def fix_format(files: tp.List[str]):
    cprinte(f"Fixing format for {len(files)} files", "yellow", attrs=["bold"])
    subprocess.run(
        ["clang-format", "-i", f"--style=file:{COURSE_DIRECTORY / '.clang-format'}"] + files
    ).check_returncode()


################################################################################


def check_current_directory_has_task():
    task_path = os.path.join(os.getcwd(), ".task.yml")

    if not os.path.isfile(task_path):
        cprinte("Current directory does not contain a task!\n"
                "Use list-tasks command to list all available tasks.",
                "red", attrs=["bold"])
        sys.exit(1)


def get_cwd_task():
    task_path = os.path.join(os.getcwd(), ".task.yml")

    with open(task_path, "r") as stream:
        task = yaml.safe_load(stream)

    return task


def get_all_cmake_targets() -> tp.Dict[str, tp.Set[str]]:
    result = {}
    for task_path in COURSE_DIRECTORY.rglob(".task.yml"):
        with open(task_path, "r") as stream:
            task = yaml.safe_load(stream)
        for target in task["cmake_targets"]:
            if result.get(target) is None:
                result[target] = set()
            for profile in task["cmake_targets"][target]["profiles"]:
                result[target].add(profile)
    return result


################################################################################

def setup_clion_workspace():
    workspace_path = COURSE_DIRECTORY / ".idea" / "workspace.xml"
    workspace_tree = ET.parse(workspace_path)

    workspace_root = workspace_tree.getroot()
    run_manager = None
    for child in workspace_root:
        if child.tag == "component" and child.attrib["name"] == "RunManager":
            run_manager = child

    if not run_manager:
        run_manager = ET.SubElement(workspace_root, "component")

    run_manager.clear()
    run_manager.attrib = {"name": "RunManager"}

    project_name = COURSE_DIRECTORY.name

    for target, profiles in get_all_cmake_targets().items():
        for profile in profiles:
            name = f"{target}.{profile}"

            run_manager_task_target = ET.SubElement(
                run_manager, "configuration", {
                    "name": name,
                    "type": "CLionExternalRunConfiguration",
                    "factoryName": "Application",
                    "REDIRECT_INPUT": "false",
                    "ELEVATE": "false",
                    "USE_EXTERNAL_CONSOLE": "false",
                    "PASS_PARENT_ENVS_2": "false",
                    "PROJECT_NAME": project_name,
                    "TARGET_NAME": name,
                    "CONFIG_NAME": name,
                    "RUN_PATH": f"$PROJECT_DIR$/build/{profile}/{target}"
                })
            run_manager_method = ET.SubElement(run_manager_task_target, "method", {"v": "2"})
            ET.SubElement(
                run_manager_method,
                "option",
                {
                    "name": "CLION.EXTERNAL.BUILD",
                    "enabled": "true"
                }
            )

    ET.indent(workspace_tree, space="  ", level=0)
    workspace_tree.write(workspace_path)


def setup_clion_targets():
    custom_targets_path = COURSE_DIRECTORY / ".idea" / "customTargets.xml"
    custom_targets_path.unlink(missing_ok=True)

    custom_targets_root = ET.Element("project", {"version": "4"})
    custom_targets_component = ET.SubElement(custom_targets_root, "component", {
                                             "name": "CLionExternalBuildManager"})

    for target, profiles in get_all_cmake_targets().items():
        for profile in profiles:
            name = f"{target}.{profile}"

            custom_targets_target = ET.SubElement(
                custom_targets_component,
                "target",
                {"name": name, "defaultType": "TOOL"}
            )

            custom_targets_configuration = ET.SubElement(
                custom_targets_target, "configuration", {"name": name})

            custom_targets_build = ET.SubElement(
                custom_targets_configuration, "build", {"type": "TOOL"})

            ET.SubElement(
                custom_targets_build, "tool", {"actionId": f"Tool_External Tools_{name}"}
            )

    custom_targets_tree = ET.ElementTree(custom_targets_root)
    ET.indent(custom_targets_tree, space="  ", level=0)
    custom_targets_tree.write(custom_targets_path)


def setup_clion_tools():
    tools_path = COURSE_DIRECTORY / ".idea" / "tools"
    if not tools_path.exists():
        tools_path.mkdir()
    toolset_path = tools_path / "External Tools.xml"
    toolset_path.unlink(missing_ok=True)
    toolset_root = ET.Element("toolSet", {"name": "External Tools"})

    for target, profiles in get_all_cmake_targets().items():
        for profile in profiles:
            name = f"{target}.{profile}"

            target_build_tool = ET.SubElement(
                toolset_root, "tool",
                {
                    "name": name,
                    "showInMainMenu": "false",
                    "showInEditor": "false",
                    "showInProject": "false",
                    "showInSearchPopup": "false",
                    "disabled": "false",
                    "useConsole": "true",
                    "showConsoleOnStdOut": "false",
                    "showConsoleOnStdErr": "false",
                    "synchronizeAfterRun": "true",
                })

            target_build_exec = ET.SubElement(target_build_tool, "exec")
            ET.SubElement(
                target_build_exec, "option",
                {
                    "name": "COMMAND",
                    "value": "nix"
                }
            )

            ET.SubElement(
                target_build_exec, "option",
                {
                    "name": "PARAMETERS",
                    "value": " --experimental-features \"nix-command flakes\" develop"
                             " $ProjectFileDir$/env --command python"
                            f" $ProjectFileDir$/env/cli build -p {profile} -t {target} --all"
                }
            )

    toolset_tree = ET.ElementTree(toolset_root)
    ET.indent(toolset_tree, space="  ", level=0)
    toolset_tree.write(toolset_path)
