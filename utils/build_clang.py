import os, sys, json, glob, subprocess, logging, shutil

from pathlib import Path
from rich.logging import RichHandler
from typing import Any, Optional

logging.basicConfig(
    level="INFO",
    format="%(message)s",
    datefmt="[%X]",
    handlers=[RichHandler(rich_tracebacks=True)]
)

log: logging.Logger = logging.getLogger("rich")

C_STANDARD: str = "clatest"
CC: str = "clang-cl.exe"
LINK: str = "link.exe"
TARGET: str = "compile_project"

#INCLUDE_MIMALLOC: str = "D:\\devTools\\mimalloc\\include\\mimalloc-3.3"
#LIB: str = "D:\\devTools\\mimalloc\\lib"

INCLUDES: str = f"/Iinclude /I"
OPTIMIZATION: str = "/Gy /fp:fast /clang:-O3 /arch:AVX512 /clang:-march=native /clang:-ffast-math"
WINVER: str = "/D_WIN32_WINNT=0x0A00"
DEBUG: str = "/fsanitize=address /Zi /O0"

STRICT_FLAGS: str = (
    "/clang:-Wall /clang:-Wextra /clang:-Wpedantic /clang:-Werror /clang:-Wuninitialized /clang:-Wold-style-definition "
    "/clang:-Wsign-conversion /clang:-Wcast-align /clang:-Wcast-qual /clang:-Wstrict-aliasing=2 /clang:-Wpointer-arith /clang:-Warray-bounds "
    "/clang:-Wnull-dereference /clang:-Wmissing-prototypes /clang:-Wstrict-prototypes /clang:-Wconversion /clang:-Wredundant-decls /clang:-Wvla "
    "/clang:-Wshadow /clang:-Wundef /clang:-Wformat=2 /clang:-Wformat-security /clang:-Wwrite-strings /clang:-Wdouble-promotion /clang:-Wfloat-equal "
    "/clang:-Wswitch-enum /clang:-Wswitch-default /clang:-Wunused /clang:-Wunused-function /clang:-Wunused-variable /clang:-Wunused-parameter "
    "/clang:-Wno-padded /clang:-Wno-declaration-after-statement /clang:-Weverything /clang:-Wno-jump-misses-init /clang:-Wno-unsafe-buffer-usage "
    "/clang:-Wno-disabled-macro-expansion /clang:-Wno-unknown-warning-option /clang:-Wno-pre-c23-compat"
)

CFLAGS: str = f"/nologo /std:{C_STANDARD} {STRICT_FLAGS} {WINVER} {INCLUDES} /MT {OPTIMIZATION}"
LDFLAGS: str = f"/nologo advapi32.lib /SUBSYSTEM:CONSOLE"

cwd: Path = Path(os.getcwd())
src: Path = cwd.joinpath("src")

build: Path = cwd.joinpath("build")
if not build.exists():
    log.info("build dir was missing, creating it...")
    os.makedirs(build, exist_ok=True)

def compile_and_link(level: str | None) -> Optional[bool]:
    compile_db: list[Any] = []
    base_args: list[str] = [CC] + CFLAGS.split()
    
    for source_file in glob.iglob(f"{src}/*.c", recursive=True):
        src_path: Path = Path(source_file)
        obj_name: str = f"{src_path.stem}.obj"
        obj_path: Path = build.joinpath(obj_name)
        
        args: list[str] = base_args + [f"/clang:-DLOG_LEVEL={level if level is not None else 1}", "/c", str(source_file), f"/Fo:{obj_path}"]
        
        compile_db.append({
            "arguments": args,
            "directory": str(cwd),
            "file": str(source_file),
            "output": str(obj_path),
        })
        
        try:
            result = subprocess.run(args, cwd=cwd, capture_output=True, check=True, text=True)
            log.info(f"Compiled src/{result.stdout.strip() if result.stdout != "" else f'{src_path.stem}.c'} => build/{obj_name}")
        except Exception as e:
            log.error(f"Failed to compile {src_path.name} due to error: {e}")
            return True
            
    with open("compile_commands.json", "w") as f:
        json.dump(compile_db, f, indent=4)
        log.info("Generated compile_commands.json")
        
    objects: str = " ".join([o for o in glob.iglob(f"{build}/*.obj", recursive=True)])
    link_args: list[str] = [LINK] + LDFLAGS.split() + [f"/out:{TARGET}.exe"] + objects.split()

    try:
        _ = subprocess.run(link_args, cwd=cwd, capture_output=True, check=True)
        log.info("Finished linking successfully")
    except Exception as e:
        log.error(f"Linking has failed due to error: {e}")
        return True
        
def cleanup() -> None:
    if build.exists():
        shutil.rmtree(build)
    
def main() -> None:
    args: list[str] = sys.argv[1:]
    
    if (compile_and_link(args[0]) == None):
        cleanup()
    
if __name__ == "__main__":
    main()