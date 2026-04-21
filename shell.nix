{ pkgs ? import <nixpkgs> {} }:

with pkgs;

# https://stackoverflow.com/questions/62592923
# https://discourse.nixos.org/t/c-and-libstdc-not-available/39126/9

(pkgs.mkShell.override {
  stdenv = pkgs.clangStdenv; # clang with GCC 15 libc++
  # stdenv = pkgs.libcxxStdenv; # clang with llvm libc++
  # stdenv = pkgs.llvmPackages_19.libcxxStdenv;
  # stdenv = pkgs.gcc13Stdenv; # GCC 13
}) {
  buildInputs = [
    cmake

/*
    clang-tools
    llvmPackages.clang
    llvmPackages.libcxx
    llvmPackages.libcxx.dev
*/

    # checkInputs
    imagemagick
    libtiff
  ];

/*
  # force libc++ usage everywhere
  shellHook = ''
    export CC=clang
    export CXX=clang++

    # ensure libc++ is actually the active STL
    export CXXFLAGS="-stdlib=libc++ -I${pkgs.llvmPackages.libcxx.dev}/include/c++/v1"
    export LDFLAGS="-stdlib=libc++"

    # prevent accidental GCC include leakage
    unset CPATH
    unset CPLUS_INCLUDE_PATH
    unset LIBRARY_PATH
  '';
  */

}
