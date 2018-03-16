#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test debug logging."""

import os

from test_framework.test_framework import BitcoinTestFramework

class LoggingTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def relative_log_path(self, name):
        return os.path.join(self.nodes[0].datadir, "regtest", name)

    def run_test(self):
        # test default log file name
        assert os.path.isfile(self.relative_log_path("debug.log"))

        # test alternative log file name in datadir
        self.restart_node(0, ["-debuglogfile=foo.log"])
        assert os.path.isfile(self.relative_log_path("foo.log"))

        # test alternative log file name outside datadir
        tempname = os.path.join(self.options.tmpdir, "foo.log")
        self.restart_node(0, ["-debuglogfile=%s" % tempname])
        assert os.path.isfile(tempname)

        # check that invalid log (relative) will cause error
        invdir = self.relative_log_path("foo")
        invalidname = os.path.join("foo", "foo.log")
        self.stop_node(0)
        self.assert_start_raises_init_error(0, ["-debuglogfile=%s" % (invalidname)],
                                                "Error: Could not open debug log file")
        assert not os.path.isfile(os.path.join(invdir, "foo.log"))

        # check that invalid log (relative) works after path exists
        self.stop_node(0)
        os.mkdir(invdir)
        self.start_node(0, ["-debuglogfile=%s" % (invalidname)])
        assert os.path.isfile(os.path.join(invdir, "foo.log"))

        # check that invalid log (absolute) will cause error
        self.stop_node(0)
        invdir = os.path.join(self.options.tmpdir, "foo")
        invalidname = os.path.join(invdir, "foo.log")
        self.assert_start_raises_init_error(0, ["-debuglogfile=%s" % invalidname],
                                               "Error: Could not open debug log file")
        assert not os.path.isfile(os.path.join(invdir, "foo.log"))

        # check that invalid log (absolute) works after path exists
        self.stop_node(0)
        os.mkdir(invdir)
        self.start_node(0, ["-debuglogfile=%s" % (invalidname)])
        assert os.path.isfile(os.path.join(invdir, "foo.log"))

        # check that -nodebuglog disables logging
        # log_path = self.relative_log_path("nosuch.log")
        # assert not os.path.isfile(log_path)
        # self.stop_node(0)
        # self.start_node(0, ["-nodebuglog", "-debuglogfile=nosuch.log"])
        # assert not os.path.isfile(log_path)

        # just sanity check no crash here
        self.stop_node(0)
        self.start_node(0, ["-debuglogfile=/dev/null"])

        # using -debuglogfile=0 should also disable logging
        self.stop_node(0)
        log_path = self.relative_log_path("0")
        assert not os.path.isfile(log_path)
        self.start_node(0, ["-debuglogfile=0"])
        assert not os.path.isfile(log_path)


if __name__ == '__main__':
    LoggingTest().main()
