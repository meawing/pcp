#!/usr/bin/env python

import click
import shutil
import sys
import typing as tp
import yaml

import lib

from termcolor import cprint

################################################################################


@click.group(cls=lib.OrderCommands)
def cli():
    """Concurrent and Parallel Programming course client."""


@cli.command()
@click.option("-p", "--profile", "profiles", multiple=True,
              help="Specify which profiles to use. This option can be used multiple times.")
@click.option("-f", "--filter", "filters", multiple=True,
              help="Specify which tests to run. This option can be used multiple times. "
              "Wildcards can also be used. For example: \"-f 'Test*' -f EdgeCase\".")
@click.option("--sandbox", is_flag=True,
              help="Run tests in an isolated environment (only for Linux).")
def test(profiles: tp.Tuple[str], filters: tp.Tuple[str], sandbox: bool):
    """Run task tests from the current directory."""
    lib.check_current_directory_has_task()
    task = lib.get_cwd_task()

    cmake_targets = task["cmake_targets"]

    failed_count = 0

    filter = ",".join(filters)

    for target in cmake_targets:
        for profile in cmake_targets[target]["profiles"]:
            if not profiles or profile in profiles:
                failed_count += not lib.run_test(target, profile, sandbox, filter)

    sys.exit(1 if failed_count else 0)


@cli.command()
def lint():
    """Run clang-tidy checks."""
    lib.check_current_directory_has_task()
    task = lib.get_cwd_task()

    cmake_targets = task["cmake_targets"]

    failed_count = 0

    profiles = set()

    for target in cmake_targets:
        for profile in cmake_targets[target]["profiles"]:
            profiles.add(profile)

    for profile in profiles:
        failed_count += not lib.run_linter(profile, task["submit_files"])

    sys.exit(1 if failed_count else 0)


@cli.command()
@click.option("--fix", is_flag=True, help="Fix format errors.")
def format(fix: bool):
    """Run clang-format checks."""
    lib.check_current_directory_has_task()
    task = lib.get_cwd_task()
    files = task["submit_files"]

    failed = False
    if fix:
        lib.fix_format(files)
    else:
        failed = not lib.run_format(files)

    if failed:
        lib.cprinte("Use `cli format --fix` to fix format errors\n",
                    "yellow", attrs=["bold"])

    sys.exit(failed)


@cli.command()
def clean():
    """Remove build files."""
    shutil.rmtree(lib.COURSE_DIRECTORY / "build", ignore_errors=True)

    # TODO(apollo1321): Remove the following code later.
    for path in lib.COURSE_DIRECTORY.iterdir():
        if path.is_dir() and path.name.startswith("build-"):
            shutil.rmtree(path)


@cli.command()
def list_tasks():
    """List all available course tasks."""
    print("\n".join(str(path.parent) for path in lib.COURSE_DIRECTORY.rglob(".task.yml")))


@cli.command()
def configure():
    """Configure all profiles."""
    profiles = set()

    for task_path in lib.COURSE_DIRECTORY.rglob(".task.yml"):
        with open(task_path, "r") as stream:
            task = yaml.safe_load(stream)

        cmake_targets = task["cmake_targets"]

        for target in cmake_targets:
            for profile in cmake_targets[target]["profiles"]:
                profiles.add(profile)

    for profile in profiles:
        lib.configure(profile)


@cli.command()
@click.option("-p", "--profile", "profiles", multiple=True,
              help="Specify which profiles to compile. This option can be used multiple times.")
@click.option("-t", "--target", "targets", multiple=True,
              help="Specify which targets to compile. This option can be used multiple times.")
@click.option("--all", "build_all", is_flag=True,
              help="Build all tasks.")
def build(profiles: tp.Tuple[str], targets: tp.Tuple[str], build_all=False):
    """Build task executable(s)."""
    if build_all:
        cmake_targets = lib.get_all_cmake_targets()
    else:
        lib.check_current_directory_has_task()
        task = lib.get_cwd_task()
        cmake_targets = {
            target: [profile for profile in task["cmake_targets"][target]["profiles"]]
            for target in task["cmake_targets"]
        }

    for target, target_profiles in cmake_targets.items():
        if not targets or target in targets:
            for profile in target_profiles:
                if not profiles or profile in profiles:
                    lib.build_executable(target, profile)


@cli.command()
def setup_clion():
    """Setup CLion targets."""
    if not (lib.COURSE_DIRECTORY / ".idea").exists():
        lib.cprinte("Idea project does not exist. Please open the project in CLion first.",
                    "red", attrs=["bold"])
        sys.exit(1)

    lib.setup_clion_tools()
    lib.setup_clion_workspace()
    lib.setup_clion_targets()


@cli.command(hidden=True)
def welcome():
    cprint("\nWelcome to Parallel and Concurrent Programming course shell!\n"
           "Print 'cli --help' to list available commands and 'exit' to quit.\n",
           "green", attrs=["bold"])


################################################################################


if __name__ == "__main__":
    cli()
