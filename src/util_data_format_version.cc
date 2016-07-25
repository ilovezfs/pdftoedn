#include "base_types.h"
#include "util_versions.h"

namespace pdftoedn {
    namespace util {
        namespace version {
            const pdftoedn::Symbol SYMBOL_DATA_FORMAT_VERSION = "data_format_version";

            uintmax_t data_format_version()
            {
                //
                // * Delorean releases did not carry data format version.
                //
                // * Delorean was renamed to Edsel. The first two
                //   Edsel releases (v0.12.0 and v0.12.1) didn't carry
                //   a data format version. From v0.13.0, this is now
                //   returned as part of the data
                //
                //   format is:
                //   xxxx yyyy
                //   where xxxx is an incremental format version bumped by
                //              one with every change (v0.13.0 began w/ 0)
                //         yyyy is the release it first included it in
                //
                //   so to compare a version bump, the lower 4 digits
                //   can be masked to just compare the upper xxxx
                //   value
                //
                // Released format changes:
                // 0000 0130:  9/24/14, v0.13.0
                //             - uniform representation of spans, images, paths
                // 0001 0220:  10/30/15, v0.22.0
                //             - image blobs exported to disk (TESLA-7128)
                //
                // * Edsel public release: app was converted to a
                //   native c++ application and renamed to pdftoedn.
                //
                // 0002 0302:  07/19/16, v0.30.2
                //             - rotated text spans now return empty
                //               x-position vectors
                //               (No data runs were done with this
                //               version)
                // 0003 0310:  07/22/16, v0.31.0
                //             - path commands were simplified:
                //               command list is no longer
                //               double-nested array and each command
                //               is now contained in a vector instead
                //               of a hash
                return 0x30310;
            }
        } // version
    } // util
} // namespace
