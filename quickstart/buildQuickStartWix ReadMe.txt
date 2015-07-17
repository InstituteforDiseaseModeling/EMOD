-----=====##### Template for using buildQuickStartWix.py #####=====-----

buildQuickStartWix.py --help
usage: buildQuickStartWix.py [-h] [-t TEMPLATE] [-b BUILDDIR] -s SAMPLESDIR -i
                             INPUTDIR [-o OUTPUT]

optional arguments:
  -h, --help            show this help message and exit
  -t TEMPLATE, --template TEMPLATE
  -b BUILDDIR, --builddir BUILDDIR
  -s SAMPLESDIR, --samplesdir SAMPLESDIR
  -i INPUTDIR, --inputdir INPUTDIR
  -o OUTPUT, --output OUTPUT


Template (-t), builddir (-b), and output (-o) are optional.
Samplesdir (-s) and inputdir (-i) are required arguments.

-----===== Minimal invocation =====-----

buildQuickStartWix.py --samplesdir ..\regression\scenarios --inputdir c:\src\input\samplesinput

This will use the defaults for template "product.xml", builddir ".", and output "product.wxs".

-----===== Full invocation =====-----

buildQuickStartWix.py --template product.xml --builddir . --samplesdir ..\regression\scenarios --inputdir c:\src\input\samplesinput --output product.wxs
