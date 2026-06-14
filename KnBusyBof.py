from os.path       import dirname
from pyhavoc.agent import *
from pyhavoc.core  import *
import json, os

#
# thanks to claude for generating this script for me lol
#

def make_bof_class( name: str, description: str, arguments: list, entry_point: str, file_path_template: str ):
    """
    Dynamically creates and registers a HcKaineCommand subclass.
    file_path_template: e.g. '/path/to/bf-env/env.{arch}.obj'
    """

    def __init__( self, *args, **kwargs ):
        super( self.__class__, self ).__init__( *args, **kwargs )
        arch             = self.agent().agent_meta()[ 'arch' ].lower()  # x64 or aarch64
        self.object_path = file_path_template.format( arch=arch )

    @staticmethod
    def arguments_fn( parser ):
        if arguments:
            lines  = [ 'arguments:' ]
            lines += [ f'  {a["name"]}{"  (optional)" if a.get("optional") else ""}  —  {a.get("desc", "")}' for a in arguments ]
            parser.epilog = '\n'.join( lines )

        parser.prefix_chars = '\x00'   # nothing looks like a flag
        parser.add_argument( 'args', nargs='*', help='' )

    async def execute( self, args ):

        task = self.agent().object_execute(
            self.object_path,
            entry_point,

            ' '.join( args.args ),

            flag_capture = True,
        )

        self.log_task( task.task_uuid(), f'executing command {name} with args: {" ".join(args.args)}' )

        try:
            handle, output = await task.result()
            self.log_info( f'({task.task_uuid():x}) received output from {name} [{len(output)} bytes]', task_id = task.task_uuid() )
            self.log_raw( output.decode(), task_id = task.task_uuid() )
        except Exception as e:
            self.log_error( f'({task.task_uuid():x}) {name} failed: {e}', task_id = task.task_uuid() )

        self.log_info( f'({task.task_uuid():x}) successfully executed {name}', task_id = task.task_uuid() )

    # Build the class dynamically
    cls = type(
        name,                          # class name
        ( HcKaineCommand, ),           # base class
        {
            '__init__' : __init__,
            'arguments': arguments_fn,
            'execute'  : execute,
        }
    )

    # Apply the decorator manually — same as @KnRegisterCommand(...)
    cls = KnRegisterCommand(
        command     = name,
        description = description,
        group       = 'BusyBox BOFs',
        platform    = 'Linux',
    )( cls )

    return cls


def load_bof_commands( base_dir: str ):
    """
    Walks bf-* subdirs, reads extension.json, registers a command per entry.
    """
    registered = []

    for entry in sorted( os.listdir( base_dir ) ):
        if not entry.startswith( 'bf-' ):
            continue

        json_path = os.path.join( base_dir, entry, 'extension.json' )
        if not os.path.exists( json_path ):
            continue

        with open( json_path ) as f:
            spec = json.load( f )

        for cmd in spec.get( 'commands', [] ):
            command_name = cmd[ 'command_name' ]
            entry_point  = cmd.get( 'entrypoint', '' )
            description  = cmd.get( 'help', '' )
            arguments    = cmd.get( 'arguments', [] )

            # Build path template using the dir — arch filled in at runtime
            # Expects: bf-env/env.x64.obj and bf-env/env.aarch64.obj
            base         = entry.removeprefix( 'bf-' )
            path_tmpl    = os.path.join( base_dir, entry, f'{base}.{{arch}}.obj' )

            cls = make_bof_class( command_name, description, arguments, entry_point, path_tmpl )
            registered.append( cls )

    return registered


# ── Entry point ───────────────────────────────────────────────
load_bof_commands( os.path.join( dirname( __file__ ), 'build' ) )
