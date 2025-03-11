{
  description = "Pararallel and Concurrent Programming course development environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    with flake-utils.lib; eachSystem [ system.x86_64-linux system.x86_64-darwin ] (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};

        # The list of packages is available here: https://search.nixos.org/packages
        devPackages = with pkgs; [
          cmake
          ninja
          gdb
          git
          which
          tl-expected
          llvmPackages_17.bintools # Utilities for backtrace symbolization.
          (lib.lists.optional stdenv.isLinux bubblewrap) # Utility to run tests in an isolated environment.
          # Place your packages here if you need any.
        ];

        cliPackages = with pkgs.python311Packages; [
          python
          click
          pyyaml
          termcolor
        ];

        # This workaround is used in darwin to enable TSAN support.
        # Interestingly, the support was initially disabled due to some licensing issues.
        # https://github.com/NixOS/nixpkgs/blob/master/pkgs/development/compilers/llvm/17/compiler-rt/default.nix#L103
        compiler-rt = pkgs.llvmPackages_17.compiler-rt.overrideAttrs (finalAttrs: previousAttrs: {
          postPatch = "";
          cmakeFlags = nixpkgs.lib.lists.remove "-DDARWIN_macosx_OVERRIDE_SDK_VERSION=ON" previousAttrs.cmakeFlags;
          buildInputs = previousAttrs.buildInputs ++ [ pkgs.darwin.libobjc ];
        });
        libraries = pkgs.llvmPackages_17.libraries // { inherit compiler-rt; };
        llvm = pkgs.llvmPackages_17.override { targetLlvmLibraries = libraries; };

        # Use the clang shell. It already includes clang, clangd, clang-format, and clang-tidy.
        mkClangShell = with pkgs; mkShell.override {
          stdenv =
            if stdenv.isDarwin then
              overrideCC clang17Stdenv llvm.clang
            else
              clang17Stdenv;
        };
      in
      {
        devShells.default = mkClangShell {
          shellHook = builtins.readFile (./shell_hook.sh);

          packages = devPackages ++ cliPackages;
        };
      }
    );
}
