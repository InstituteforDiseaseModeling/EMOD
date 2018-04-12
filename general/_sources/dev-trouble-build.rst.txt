=================================
Troubleshooting |EMOD_s| builds
=================================

If you encounter any of the following warnings or errors when attempting to build the |exe_l| or
|linux_binary|, see the information below to resolve the issue.

If you need assistance, you can contact support for help with solving issues. You can contact
|IDM_l| support at support@idmod.org. When submitting the issue, please include any error
information.

Unknown compiler version
========================

If you encounter the warning "Unknown compiler version - please run the configure tests and report
the results" when attempting to build the |exe_s| or |linux_binary|, this indicates you are using a
version of Boost that is no longer supported. Install |Boost_supp|.

Inconsistent DLL linkage
========================

If you see the following warning on some files, "c:python27includepymath.h(22): warning C4273:
‘round’: inconsistent dll linkage", this indicates that you are using a version of Python that is
no longer supported. Install |Python_supp|.

Error 255
=========

Check to see if you have any white spaces in the path to your local |EMOD_s| source code. If you do,
remove the white spaces in all of the directory names in the path.

Error LNK4272
=============

If you see the error "LNK4272 library machine type 'X86' conflicts with target machine type 'x64'",
this indicates that you need to uninstall 32-bit Python and reinstall 64-bit Python. Install
|Python_supp|.
