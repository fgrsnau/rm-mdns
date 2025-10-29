let
  pkgs = import <nixpkgs> { };
in

pkgs.mkShell {
  packages = [
    pkgs.gcc
    pkgs.meson
    pkgs.ninja

    pkgs.clang-tools
    pkgs.nixfmt-rfc-style
    pkgs.nodePackages.prettier
    pkgs.vim
  ];
}
