// argument #1 - filename for version info scratch file (current data)
// argument #2 - filename for version_info template (source template)
// argument #3 - filename for version_info include file (destination file)

// Supported macros:
// $BUILDER$   - builder [user]name
// $BRANCH$   - should be commit branch
// $HASH$     - e.g. git SHA1 hash for current commit
// $DATE$     - commit date
// $REVISION$ - revision number
// Deprecated: $NOW$    - current date

version_info_filename = WScript.Arguments.Unnamed.Item(0);
template_filename     = WScript.Arguments.Unnamed.Item(1);
header_filename       = WScript.Arguments.Unnamed.Item(2);

WScript.Echo("version info:    " + version_info_filename);
WScript.Echo("template source: " + template_filename);
WScript.Echo("header file:     " + header_filename);

// Read the version template file (version_info.tmpl)
var fso = new ActiveXObject("Scripting.FileSystemObject");
f = fso.OpenTextFile(template_filename, 1 /* ForReading */);
template = f.ReadAll();
f.close();

f = fso.OpenTextFile(version_info_filename, 1 /* ForReading */);
builder      = f.ReadLine();
sccs_branch  = f.ReadLine();
sccs_hash    = f.ReadLine();
sccs_date    = f.ReadLine();
commit_count = f.ReadLine();
f.close()

WScript.Echo("builder: " + builder)
WScript.Echo("branch:  " + sccs_branch)
WScript.Echo("commit:  " + sccs_hash)
WScript.Echo("date:    " + sccs_date)
WScript.Echo("build:   " + commit_count)

function escapeRegExp(str) {
    return str.replace(/([.*+?^=!:${}()|\[\]\/\\])/g, "\\$1");
}

function replaceAll(str, find, replace) {
    return str.replace(new RegExp(escapeRegExp(find), 'g'), replace);
}

template = replaceAll(template, '$BUILDER$', builder);
template = replaceAll(template, '$BRANCH$', sccs_branch);
template = replaceAll(template, '$HASH$', sccs_hash);
template = replaceAll(template, '$DATE$', sccs_date);
template = replaceAll(template, '$REVISION$', commit_count);
// Prefer __DATE__ and __TIME__ macros from the compiler.
// now = new Date();
// template = replaceAll(template, '$NOW$', now)

// Write header file (version_file.h)
s = fso.CreateTextFile(header_filename, true /* overwrite */);
s.Write(template);
s.close();