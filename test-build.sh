#!/usr/bin/env bash

declare -gr extra_cflags='-fdiagnostics-color=always'

declare -agr compilers=(
	clang-{20,1{9,8,7,6,5,4,3,2,1,0},9,8,7,6.0}
	gcc-{1{4,3,2,1,0},9,8,7}
)

declare -Agr build_types=(
	['debug']='Debug'
	['release']='Release'
	['relwithdebinfo']='RelWithDebInfo'
	['minsizerel']='MinSizeRel'
)

declare -Agr cflags=(
	['clang-6.0']='-march=znver1 -mtune=znver1'
	['clang-7']='-march=znver1 -mtune=znver1'
	['clang-8']='-march=znver1 -mtune=znver1'
	['clang-9']='-march=znver2 -mtune=znver2'
	['clang-10']='-march=znver2 -mtune=znver2'
	['clang-11']='-march=znver2 -mtune=znver2'
	['clang']='-march=znver3 -mtune=znver3'
	['gcc-7']='-march=znver1 -mtune=znver1'
	['gcc-8']='-march=znver1 -mtune=znver1'
	['gcc-9']='-march=znver2 -mtune=znver2'
	['gcc']='-march=znver3 -mtune=znver3'
)

declare -Agr wflags=(
	['clang']='-Wall -Wextra -Wpedantic'
	['gcc']='-Wall -Wextra -Wpedantic'
)

do_build()
{
	local base_dir bt build_type cc cc_type compiler \
	      logs_dir name ret src_dir work_dir x

	compiler=$(command -v "$1") || {
		printf "Invalid compiler command: %q\n" "$1" >&2
		return 1
	}

	bt="${2,,}"
	build_type="${build_types[$bt]}"
	[[ "$build_type" ]] || {
		printf "Build type doesn't exist: %q\n" "$2" >&2
		return 1
	}

	src_dir=$(realpath -e "$3") || {
		printf "Source dir doesn't exist: %q\n" "$3" >&2
		return 1
	}

	base_dir=$(realpath -e "${4:-.}") || {
		printf "Build root doesn't exist: %q\n" "$4" >&2
		return 1
	}

	cc="${compiler##*/}"
	cc_type="${cc%-*}"
	cc_flags="${cflags[$cc]:-${cflags[$cc_type]}}"
	warnings="${wflags[$cc]:-${wflags[$cc_type]}}"

	name="${src_dir##*/}/$bt/$cc"
	logs_dir="$base_dir/logs/$name"
	work_dir="$base_dir/work/$name"

	local -a cmdline_options=(
		CMAKE_BUILD_TYPE="$build_type"
		CMAKE_C_COMPILER="$compiler"
		CMAKE_C_FLAGS="$cc_flags $extra_cflags $warnings"
		CMAKE_COLOR_MAKEFILE=ON
		CMAKE_EXPORT_COMPILE_COMMANDS=ON
		CMAKE_VERBOSE_MAKEFILE=ON
	)

	mkdir -p "$work_dir" "$logs_dir" || return 1

	printf -vx "%0${#name}d" '0'; x="${x//0/─}"
	printf -vx "╭─$x─╮\n│ %s │\n╰─$x─╯" "$name"

	echo "$x" > "$logs_dir/config.log"

	ret=1

	if ! { cmake -B "$work_dir" -S "$src_dir" \
	             "${cmdline_options[@]/#/-D}" \
	       >> "$logs_dir/config.log" 2>&1;    }
	then
		echo 'CONFIG ERROR' >> "$logs_dir/config.log"
	else
		echo "$x" > "$logs_dir/build.log"
		if ! { cmake --build "$work_dir" -j 1 \
		       >> "$logs_dir/build.log" 2>&1; }
		then
			echo 'BUILD ERROR' >> "$logs_dir/build.log"
		else
			ret=0
		fi
	fi

	if (( ret )); then
		printf '[\033[31mEE\033[m] %s\n' "$name"
	else
		printf '[\033[32mOK\033[m] %s\n' "$name"
	fi

	return $ret
}

repo=$(realpath "${1:-.}") && [[ -d "$repo/bin/configs" ]] || {
	echo "Can't find configs dir" >&2
	exit 1
}

B='[[:blank:]]'
src_dirs=($(grep -EZl "^$B*generatorName:$B*c$B*$" \
                      "$repo/bin/configs/"*.yaml   \
            | xargs -0 grep -Eh "^$B*outputDir:"   \
            | sed -E "s,^$B*outputDir:$B*,$repo/,"))
(( ${#src_dirs[@]} )) || {
	echo "Can't find any sources" >&2
	exit 1
}

tmp_dir=$(mktemp -dp /tmp XXXX) && [[ -d "$tmp_dir" ]] || {
	echo "Can't create temp dir" >&2
	exit 1
}

do_parallel_build()
{
	local -i m="$1" n="$2" i=0
	(( m > 0 && m > n )) || {
		return 1
	}
	local c b d
	for c in "${compilers[@]}"; do
		for b in "${build_types[@]}"; do
			for d in "${src_dirs[@]}"; do
				(( (i % m) != n )) || {
					do_build "$c" "$b" "$d" "$tmp_dir"
				}
				((i++))
			done
		done
	done
}

declare -i i=0 n=$(nproc)
(( (n >= 1) || (n = 1) ))
for (( ; i < n; i++)); do
	do_parallel_build "$n" "$i" &
done 2>/dev/null
wait 2>/dev/null

rsync -av                               \
      --prune-empty-dirs                \
      --include='*/'                    \
      --include='compile_commands.json' \
      --exclude='*'                     \
      "$tmp_dir/work/"                  \
      "$tmp_dir/logs/" >/dev/null 2>&1
find "$tmp_dir/logs/" -type f

# Write functions to GITHUB_ENV encoded as variables
save_functions() {
	local -a v=('' "$@")
	set - "${@/#/\"\$(declare -f }"
	set - "${@/%/\)\"}"
	eval "set - $@"
	[[ "$GITHUB_ENV" ]] || local GITHUB_ENV=/dev/stdout
	local -i i
	for ((i=$#; i; --i)); do
		echo "__saved_func_${v[i]}=${!i@Q}" >> "$GITHUB_ENV"
	done
}

# Decode and define functions loaded from GITHUB_ENV
load_functions() {
	set - ${!__saved_func_*}
	set - "${@/#/eval \"\$}"
	eval "${@/%/\";}"
	unset -v ${!__saved_func_*}
}
