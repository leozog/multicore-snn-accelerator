import inspect
import pathlib

# Constructs an output path based on the caller's location and ensures the directory exists
# param file_name: Name of the file to be saved in the output directory
# param print_info: If True, prints the saving information
# return: Full path to the output file as a string
# throws: if no "scripts" directory found in the caller's path
def output_path(file_name: str, print_info: bool = True) -> str:
    stack_frame_idx = 1
    while True:
        caller_file = pathlib.Path(inspect.stack()[stack_frame_idx].filename).resolve()
        caller_dir = caller_file.parent
        path = caller_dir
        while path.name != "scripts":
            if path == path.parent:
                break
            path = path.parent
        if path.name == "scripts":
            break
        stack_frame_idx += 1
    scripts_dir = path
    project_root = scripts_dir.parent
    rel_path = caller_dir.relative_to(scripts_dir)
    out_dir = project_root / "out" / rel_path
    out_dir.mkdir(parents=True, exist_ok=True)
    final_path = out_dir / file_name
    final_dir = final_path.parent
    final_dir.mkdir(parents=True, exist_ok=True)
    if print_info:
        print(f"Saving {file_name}\tin {final_dir}")
    return str(final_path)