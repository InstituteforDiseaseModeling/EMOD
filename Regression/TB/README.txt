Put Eradication.exe in this folder, and if you want to look at the property output, copy plotPropertyReport into here.

Eradication.exe -C MDRDiagnostic/config_one_day_latent_MDR.json -O MDRDiagnostic/output_mdr > MDRDiagnostic/console-mdr.txt

>Eradication.exe -C MDRDiagnostic/config_one_day_latent_MDR-fiveProperty.json -O MDRDiagnostic/output_mdr-fiveProperty > MDRDiagnostic/console-mdr-fiveProperty.txt
>plotPropertyReport.py -c "Statistical Population" MDRDiagnostic\output_mdr-fiveProperty\PropertyReportTB.json

Eradication.exe -C ActiveDiagnostic/config_one_day_latent_active.json -O ActiveDiagnostic/output-one-day-latent > ActiveDiagnostic/console-one-day-latent.txt

Eradication.exe -C SmearDiagnostic/config_one_day_latent_smear.json -O SmearDiagnostic/output-smear-diagnostic > SmearDiagnostic/console-smear-diagnostic.txt


Eradication.exe -C SLDDiagnostic/config_one_day_latent_SLD.json -O SLDDiagnostic/output-sld > SLDDiagnostic/console-sld.txt

Eradication.exe -C SLDDiagnostic/config_one_day_latent_SLD_neg_config.json -O SLDDiagnostic/output-sld-neg-config > SLDDiagnostic/console-sld-neg-config.txt

Eradication.exe -C SLDDiagnostic/config_one_day_latent_SLD-fiveProperty.json -O SLDDiagnostic/output-sld-fiveProperty > SLDDiagnostic/console-sld-fiveProperty.txt
plotPropertyReport.py SLDDiagnostic/output-sld-fiveProperty/PropertyReportTB.json -c "Statistical Population"

ERAD-1909
Eradication.exe -C HTIV-Scaleup\config_MDR_scaleup_property.json -O HTIV-Scaleup\output_scaleup_property > HTIV-Scaleup\console-scaleup-property.txt
plotPropertyReport.py -c "Statistical Population" HTIV-Scaleup\output_scaleup_property\PropertyReportTB.json

