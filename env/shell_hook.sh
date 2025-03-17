export COURSE_DIRECTORY=$(git rev-parse --show-toplevel)
export ASAN_SYMBOLIZER_PATH=$(which llvm-symbolizer)
export TSAN_SYMBOLIZER_PATH=$(which llvm-symbolizer)

alias cli='python $COURSE_DIRECTORY/env/cli'

shopt -s expand_aliases

cli welcome 2>/dev/null

PS1="(pcp-env) $PS1"
