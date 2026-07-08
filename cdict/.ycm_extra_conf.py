import os
import subprocess
import ycm_core

DIR_OF_THIS_SCRIPT = os.path.abspath( os.path.dirname( __file__ ))



def Settings( **kwargs ):
    return {
        'flags': [ '-x', 'c', '-I', "./include" ],
        'include_paths_relative_to_dir': DIR_OF_THIS_SCRIPT
    }
# end def
