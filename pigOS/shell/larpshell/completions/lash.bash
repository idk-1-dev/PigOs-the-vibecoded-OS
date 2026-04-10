# lash completion                                        -*- shell-script -*-

_comp_cmd_lash()
{
    local cur prev words cword comp_args
    _comp_initialize -- "$@" || return

    case $prev in
        -c|--command)
            _comp_compgen_command
            return
            ;;
        -s|--shell)
            comps=$(compgen -c)
            _comp_compgen -a -- "$comps"
            return
            ;;
        -h|--help|-v|--version)
            return
            ;;
    esac

    if [[ $cur == -* ]]; then
        _comp_compgen -- -W '-c --command -s --shell -h --help -v --version -i --interactive -n --no-profile -C --cwd'
        return
    fi

    _comp_compgen_filedir
}
