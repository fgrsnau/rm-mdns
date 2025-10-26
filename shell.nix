let
  pkgs = import <nixpkgs> { };
in

pkgs.mkShell {
  packages = [
    pkgs.gcc
    pkgs.meson
    pkgs.ninja

    pkgs.vim
    pkgs.clang-tools
    pkgs.nixfmt-rfc-style
  ];
}
