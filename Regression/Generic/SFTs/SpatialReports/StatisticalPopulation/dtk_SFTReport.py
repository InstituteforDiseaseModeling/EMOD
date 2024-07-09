#!/usr/bin/python

import contextlib
import traceback
import sys
import dtk_test.dtk_sft as sft


class SFTReport(object):
    """
    Keep track of good/bad messages and overall success/failure, write out an scientific_feature_report.txt
    """
    def __init__(self, report_name):
        self.report_name = report_name
        self.messages = []
        self.success = True

    def add_good_msg(self, msg):
        self.add_result(True, msg)

    def add_bad_msg(self, msg):
        self.add_result(False, msg)

    def add_msg(self, msg):
        self.messages.append(msg)

    def add_result(self, result, msg=''):
        self.success &= result
        if msg:
            if result:
                self.add_msg('GOOD: ' + msg)
            else:
                self.add_msg('BAD: ' + msg)
        elif not result:
            # add a stack trace when adding a failure without a message
            self.add_msg('Unknown failure: ' + ''.join(traceback.format_stack()))

    def add_success(self):
        self.add_result(True)

    def add_failure(self):
        self.add_result(False)

    @contextlib.contextmanager
    def use_report(self):
        try:
            yield
        except:
            self.success = False
            t, v, tb = sys.exc_info()
            formatted_tb = '\n'.join(traceback.format_tb(tb))
            self.messages.append(f'{t} exception occured: {v}')
            self.messages.append(formatted_tb)
        finally:
            with open(self.report_name, 'w') as report_file:
                for message in self.messages:
                    report_file.write(message + '\n')
                report_file.write(sft.format_success_msg(self.success))
