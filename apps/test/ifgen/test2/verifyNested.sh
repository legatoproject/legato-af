#
# Simple script to verify the generated nested interface files.
#
# Usage: verifyNested.sh <dir>
#
# where <dir> is the output directory that is then compared against generated_nested
#

destdir=$1

ifgen nested.api --gen-interface --output-dir $destdir
ifgen nested2.api --gen-interface --output-dir $destdir
ifgen nested3.api --gen-interface --output-dir $destdir
ifgen nested4.api --gen-interface --output-dir $destdir

diff -r generated_nested $destdir
