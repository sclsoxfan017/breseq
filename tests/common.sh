#!/bin/bash
#
# Common testing functionality for breseq.
#
# Testing in breseq is based on matching sha1 hashes of files ('cause
# the data itself is too big to want to store in hg).  But, some of the output
# includes dates, so we only hash certain files.

# common variables:
# paths must either be relative to the location of this script or absolute.
COMMONDIR=`dirname ${BASH_SOURCE}`
source ${COMMONDIR}/test.config

# path to breseq:
if [ "$TESTBINPREFIX" == "" ]
then
	echo "Environmental variable \$TESTBINPREFIX not defined."
	exit
fi

BRESEQ="${TESTBINPREFIX}/breseq -j 2"
GDTOOLS="${TESTBINPREFIX}/gdtools"

# path to test data:
DATADIR=${COMMONDIR}/data
# this is a find-compatible list of files that we'll hash:
#FILE_PATTERN='( -name *.tab -or -name *.html ) -and -not -name settings.tab -and -not -name summary.tab'
FILE_PATTERN='-name output.gd'
# executable used to hash files:
HASH=`which sha1sum`
# executable used to diff files
DIFF_BIN=`which diff`
# name of testexec file
TESTEXEC=testcmd.sh

# build the expected results from the current output
# $1 == testdir
#
do_build() {
    pushd $1 > /dev/null
#    for i in `find . ${FILE_PATTERN}`; do
#       ${HASH} $i
#    done > ${EXPECTED}
	for (( i=0; i<${#EXPECTED_OUTPUTS[@]}; i++ )); do
		echo "cp ${CURRENT_OUTPUTS[$i]} ${EXPECTED_OUTPUTS[$i]}"
    	cp ${CURRENT_OUTPUTS[$i]} ${EXPECTED_OUTPUTS[$i]}
	done
    popd > /dev/null
}


# show the files that will be checked
# $1 == testdir
#
do_show() {
    pushd $1 > /dev/null
    find . ${FILE_PATTERN}
    popd > /dev/null
}


# check current results against expected values.
# $1 == testdir
#
do_check() {
	NEEDS_UPDATING=0
    for EXPECTED_OUTPUT in "${EXPECTED_OUTPUTS[@]}"; do  
		if [[ ! -e ${1}/${EXPECTED_OUTPUT} ]]; then
			NEEDS_UPDATING=1
		fi
	done

	if [ ${NEEDS_UPDATING} == 1 ]; then
		echo ""
		do_build $1
		echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
		echo "Built expected output"
		echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
	fi
#We need to check to see if all the output files even exist 
#    for i in `find . ${FILE_PATTERN}`; do
#   	echo "$i"
#        if [[ ! -e $i ]]; then
#        	echo ""
#			echo "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
#			echo "Failed check: $1"
#        	echo "Did not find expected output file: $i"
#			echo "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" 
#			echo ""
#       	exit -1
#       fi
#    done
    pushd $1 > /dev/null
    echo ""
#   CHK=`${HASH} -s --check ${EXPECTED} 2>&1`
	for (( i=0; i<${#EXPECTED_OUTPUTS[@]}; i++ )); do
		echo "Comparing files: ${CURRENT_OUTPUTS[$i]} ${EXPECTED_OUTPUTS[$i]}"  
		CHK=`${DIFF_BIN} ${CURRENT_OUTPUTS[$i]} ${EXPECTED_OUTPUTS[$i]}`
		if [[ "$?" -ne 0 || $CHK ]]; then
			echo "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
			echo "Failed check"
			#${HASH} --check ${EXPECTED}
			${DIFF_BIN} ${CURRENT_OUTPUTS[$i]} ${EXPECTED_OUTPUTS[$i]}
			echo "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" 
			echo ""
			popd > /dev/null        
		else
			echo "OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO"
			echo "Passed check"
			echo "OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO"
			echo ""
			popd > /dev/null    
		fi
	done
}

do_breseq() {
	echo "BRESEQ COMMAND:" 
	echo $TESTCMD
	$TESTCMD
	if [[ "$?" -ne 0 ]]; then
	    echo "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
        echo "Failed check"
        echo "Non-zero error code returned."
        echo "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" 
        exit -1
	fi
}

# verbose check of current hashes against expected values.
# $1 == testdir
do_vcheck() {
    if [[ ! -e $1/${EXPECTED} ]]; then
        echo "Building expected values for test $1..."
        do_build $1
    fi
    pushd $1 > /dev/null
    if ! ${HASH} --check ${EXPECTED}; then
        popd > /dev/null
        echo "Failed check: $1"
        exit -1
    fi
    popd > /dev/null
    echo "Passed check: $1"
    exit 0
}



# clean a test
# $1 == testdir
#
do_clean() {
    for CURRENT_OUTPUT in "${CURRENT_OUTPUTS[@]}"; do
		rm -f ${1}/${CURRENT_OUTPUT}
	done
	rm -Rf $1/0* $1/output $1/data $1/output.gff3

}


# main test-running method
# $1 == action
# $2 == testdir
# testcmd == function that must be defined in the test command file.
#
do_test() {

# If there is only one argument, assume "test"
	if [[ -n $2 ]]; then
		TESTDIR=$2
		TESTSUBCMD=$1
	else
		TESTDIR=$1
		TESTSUBCMD=test
	fi
	echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	echo "TEST DIRECTORY:" $TESTDIR
	echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	case $TESTSUBCMD in
        build)
            do_build $TESTDIR
        ;;
        check)
            do_check $TESTDIR
        ;;
        clean)
            do_clean $TESTDIR
        ;;
        rebuild)
            do_clean $TESTDIR
            do_breseq
            do_build $TESTDIR
        ;;
        show)
            do_show $TESTDIR
        ;;
        test)
        	do_breseq
            do_check $TESTDIR
        ;;
        vcheck)
            do_vcheck $TESTDIR
        ;;
        *)
            do_usage
        ;;
	esac
}

# print usage information.
#
do_usage() {
    echo "Usage: $0 build|check|clean|rebuild|test <testdir>" >&2
    exit -1
}

