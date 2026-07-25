import os, json, glob, subprocess, logging, shutil

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
CC: str = "cl.exe"
LINK: str = "link.exe"
TARGET: str = "compile_project"

#INCLUDE_MIMALLOC: str = "D:\\devTools\\mimalloc\\include\\mimalloc-3.3"
#LIB: str = "D:\\devTools\\mimalloc\\lib"

INCLUDES: str = f"/Iinclude /I"
OPTIMIZATION: str = "/GL /fp:fast /O2 /arch:AVX2"
WINVER: str = "/D_WIN32_WINNT=0x0A00"
DEBUG: str = "/fsanitize=address /Zi /O0"

STRICT_FLAGS: str = "/W4 /permissive- /w14456 /w14457 /w14668 /w14061 /w14062 /w14244 /w14242 /w14018"
CFLAGS: str = f"/nologo /std:{C_STANDARD} {STRICT_FLAGS} {WINVER} {INCLUDES} /MT {OPTIMIZATION}"
LDFLAGS: str = f"/nologo /LTCG advapi32.lib /SUBSYSTEM:CONSOLE"

cwd: Path = Path(os.getcwd())
src: Path = cwd.joinpath("src")

build: Path = cwd.joinpath("build")
if not build.exists():
    log.info("build dir was missing, creating it...")
    os.makedirs(build, exist_ok=True)

def compile_and_link() -> Optional[bool]:
    compile_db: list[Any] = []
    
    # Pre-calculate your shared flags to avoid duplication
    # Using a list instead of a string is better for the "arguments" field
    base_args = [CC] + CFLAGS.split()
    
    for source_file in glob.iglob(f"{src}/*.c", recursive=True):
        src_path: Path = Path(source_file)
        obj_name: str = f"{src_path.stem}.obj"
        obj_path: Path = build.joinpath(obj_name);
        
        exec_command: str = f"{CC} {CFLAGS} /c {source_file} /Fo:{obj_path}"
        args = base_args + ["/c", str(source_file), f"/Fo:{obj_path}"]
        
        compile_db.append({
            "arguments": args,
            "directory": str(cwd),
            "file": str(source_file),
            "output": str(obj_path),
        })
        
        try:
            result = subprocess.run(exec_command.split(' '), cwd=cwd, capture_output=True, check=True, text=True)
            log.info(f"Compiled src/{result.stdout.strip()} => build/{obj_name}")
        except Exception as e:
            log.error(f"Failed to compile {src_path.name} due to error: {e}")
            return True
            
    with open("compile_commands.json", "w") as f:
        json.dump(compile_db, f, indent=4)
        log.info("Generated compile_commands.json")
        
    objects: str = " ".join([o for o in glob.iglob(f"{build}/*.obj", recursive=True)])
    exec_command: str = f"{LINK} {LDFLAGS} /out:{TARGET}.exe {objects}"

    try:
        _ = subprocess.run(exec_command.split(' '), cwd=cwd, capture_output=False, check=True)
        log.info("Finished linking successfully")
    except Exception as e:
        log.error(f"Linking has failed due to error: {e}")
        return True
        
def cleanup() -> None:
    if build.exists():
        shutil.rmtree(build)
    
def main() -> None:
    if (compile_and_link() == None):
        cleanup()
    
if __name__ == "__main__":
    main()