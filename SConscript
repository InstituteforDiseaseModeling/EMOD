# -*- mode: python; -*-
# This Python script, SConscript, invoked by the SConstruct in this directory,
#
# 1. delegates to other per-module SConscript files for executable and library 
# (static and/or dynamic)
import os
import sys
import shutil
import pdb

Import('env')

def InstallEmodules(src, dst):
    
    print "\nInstalling from " + src + " to " + dst + "..."
    if os.path.isfile(dst):
        print "Warning: " + dst + " is a file\n";
        return;

    if os.path.exists(dst) != True:
        print "Creating " + dst + " in " + os.getcwd();
        os.mkdir(dst)

    srcfiles = os.path.join(src,'*.dll')
    for root, dirs, files in os.walk(src):
        for file in files:
            if file.endswith('.dll') or file.endswith('.exe'):
                full_fn = os.path.join(root,file);
                print "copying: " + full_fn;
                shutil.copy2(full_fn, dst);
    
# if --install is on, just copy the dlls (assumed there already) and finish
dst_path = env['Install']
if dst_path != "":
    InstallEmodules(Dir('.').abspath, dst_path)
    #InstallEmodules(Dir('#').abspath, dst_path)
    print("Finished installing.\n")
    sys.exit(0)

# set the common libraries
env.Append(LIBPATH = ["$BUILD_DIR/baseReportLib", "$BUILD_DIR/cajun", "$BUILD_DIR/campaign", "$BUILD_DIR/snappy", "$BUILD_DIR/lz4", "$BUILD_DIR/utils"])

print( "Link executable against cajun, campaign, snappy, and utils lib's." )
env.Append(LIBS=["baseReportLib", "cajun", "campaign", "snappy", "lz4", "utils"])

#print "builddir is " + env["BUILD_DIR"]

# First static libs
SConscript( [ 'baseReportLib/SConscript',
              'cajun/SConscript',
              'campaign/SConscript',
              'Eradication/SConscript_coreLib',
              'snappy/SConscript',
              'lz4/SConscript',
              'utils/SConscript' ])

# If DLL=true, build libgeneric_static.lib
# to be used by other dlls

# not sure yet exactly right set of conditions for this
#if env['AllDlls'] or ( 'AllInterventions' in env and env['AllInterventions'] ) or ( 'DiseaseDll' in env and env[ 'DiseaseDll' ] != "" ) or ( 'Report' in env and env[ 'Report' ] != "" ) or ( 'Campaign' in env and env[ 'Campaign' ] != "" ):
if env['AllDlls'] or ( 'DiseaseDll' in env and env[ 'DiseaseDll' ] != "" ) or ( 'Report' in env and env[ 'Report' ] != "" ):
    print "Build libgeneric_static.lib for dll...."
    SConscript( 'libgeneric_static/SConscript' )

# Then build dlls
if env['AllDlls']:
    print "Build all dlls..."
    SConscript( 'libgeneric/VectorSConscriptStatic' )
    SConscript( 'libgeneric/MalariaSConscriptStatic' )
    SConscript( 'libgeneric/EnvironmentalSConscriptStatic' )
    SConscript( 'libgeneric/GenericSConscript' )
    SConscript( 'libgeneric/VectorSConscript' )
    SConscript( 'libgeneric/MalariaSConscript' )
    SConscript( 'libgeneric/EnvironmentalSConscript' )
    SConscript( 'libgeneric/TBSConscriptStatic' )
    SConscript( 'libgeneric/TBSConscript' )
    SConscript( 'libgeneric/STISConscriptStatic' )
    SConscript( 'libgeneric/HIVSConscript' )
    SConscript( 'libgeneric/PySConscript' )
    #SConscript( 'libgeneric/PolioSConscript' )
elif env[ 'DiseaseDll' ] != "":
    print( "Build specific disease dll." )
    dtype = env['DiseaseDll']
    if dtype == 'Generic':
        SConscript( 'libgeneric/GenericSConscript', variant_dir="Generic/disease_plugins" )
    elif dtype == 'Vector':
        SConscript( 'libgeneric/VectorSConscriptStatic' )
        SConscript( 'libgeneric/VectorSConscript', variant_dir="Vector/disease_plugins" )
    elif dtype == 'Malaria':
        SConscript( 'libgeneric/VectorSConscriptStatic' )
        SConscript( 'libgeneric/MalariaSConscriptStatic' )
        SConscript( 'libgeneric/MalariaSConscript', variant_dir="Malaria/disease_plugins" )
    elif dtype == 'Environmental':
        SConscript( 'libgeneric/EnvironmentalSConscriptStatic' )
        SConscript( 'libgeneric/EnvironmentalSConscript', variant_dir="Environmental/disease_plugins" )
    elif dtype == 'Polio':
        SConscript( 'libgeneric/EnvironmentalSConscriptStatic' )
        SConscript( 'libgeneric/PolioSConscript', variant_dir="Polio/disease_plugins" )
    elif dtype == 'TB':
        SConscript( 'libgeneric/TBSConscriptStatic' )
        SConscript( 'libgeneric/TBSConscript', variant_dir="TB/disease_plugins" )
    elif dtype == 'STI':
        SConscript( 'libgeneric/STISConscriptStatic' )
        SConscript( 'libgeneric/STISConscript', variant_dir="STI/disease_plugins" )
    elif dtype == 'HIV':
        SConscript( 'libgeneric/STISConscriptStatic' )
        SConscript( 'libgeneric/HIVSConscriptStatic' )
        SConscript( 'libgeneric/HIVSConscript', variant_dir="HIV/disease_plugins" )
    elif dtype == 'PY':
        SConscript( 'libgeneric/PySConscript', variant_dir="Py/disease_plugins" )
    else:
        print "Unspecified or unknown disease type: " + dtype

# intervention dlls
if env['AllDlls'] or env[ 'DiseaseDll' ] != "":
    print( "Building dlls." )

    # this vector and malaria static is needed for MalariaDrugTypeParameters 
    # should be cleared out once it is done correctly
    #SConscript( 'libgeneric/VectorSConscriptStatic' )
    #SConscript( 'libgeneric/MalariaSConscriptStatic' )

    dll_op_path = env['DiseaseDll'] + "/interventions"
    # Vector
    if env['DiseaseDll'] == "Vector" or env['DiseaseDll'] == "Malaria":
        SConscript( 'libgeneric/BednetSConscript', variant_dir=dll_op_path )
        SConscript( 'libgeneric/HousingmodSConscript', variant_dir=dll_op_path )
        SConscript( 'libgeneric/HumanhostseekingtrapSConscript', variant_dir=dll_op_path )
        SConscript( 'libgeneric/IndividualrepellentSConscript', variant_dir=dll_op_path )
        SConscript( 'libgeneric/IvermectinSConscript', variant_dir=dll_op_path )
        SConscript( 'libgeneric/MosquitoreleaseSConscript', variant_dir=dll_op_path )
        SConscript( 'libgeneric/ScalelarvalhabitatSConscript', variant_dir=dll_op_path )
        SConscript( 'libgeneric/VcntSConscript', variant_dir=dll_op_path )

    # Malaria
    if env['DiseaseDll'] == "Malaria":
        SConscript( 'libgeneric/AntimalarialdrugSConscript', variant_dir=dll_op_path  )
        SConscript( 'libgeneric/InputEIRSConscript', variant_dir=dll_op_path )
        SConscript( 'libgeneric/MalariaChallengeSConscript', variant_dir=dll_op_path )
        SConscript( 'libgeneric/RTSSVaccineSConscript', variant_dir=dll_op_path )

    # TB
    if env['DiseaseDll'] == "TB":
        SConscript( 'libgeneric/ActivediagnosticsSConscript', variant_dir=dll_op_path )
        SConscript( 'libgeneric/AntitbdrugSConscript', variant_dir=dll_op_path )
        SConscript( 'libgeneric/AntitbpropdepdrugSConscript', variant_dir=dll_op_path )
        SConscript( 'libgeneric/BCGVaccineSConscript', variant_dir=dll_op_path )
        SConscript( 'libgeneric/DiagnosticstreatnegSConscript', variant_dir=dll_op_path )
        SConscript( 'libgeneric/HealthSeekingBehaviorUpdateSConscript', variant_dir=dll_op_path )
        SConscript( 'libgeneric/HealthSeekingBehaviorUpdateableSConscript', variant_dir=dll_op_path )
        SConscript( 'libgeneric/NodeLevelHealthTriggeredIVScaleUpSwitchSConscript', variant_dir=dll_op_path )
        SConscript( 'libgeneric/ResistancediagnosticsSConscript', variant_dir=dll_op_path )
        SConscript( 'libgeneric/SmearDiagnosticsSConscript', variant_dir=dll_op_path )

    if env['DiseaseDll'] == "STI" or env['DiseaseDll'] == "HIV":
        SConscript( 'libgeneric/StibarrierSConscript', variant_dir=dll_op_path )
        SConscript( 'libgeneric/StiispostdebutSConscript', variant_dir=dll_op_path )
        SConscript( 'libgeneric/ModifysticoinfectionstatusSConscript', variant_dir=dll_op_path ) 
        SConscript( 'libgeneric/SticoinfectiondiagnosticSConscript', variant_dir=dll_op_path )
        SConscript( 'libgeneric/MalecircumcisionSConscript', variant_dir=dll_op_path )

    if env['DiseaseDll'] == "HIV":
        SConscript( 'libgeneric/ArtbasicSConscript', variant_dir=dll_op_path )
        SConscript( 'libgeneric/ArtdropoutSConscript', variant_dir=dll_op_path ) 
        SConscript( 'libgeneric/Cd4diagnosticSConscript', variant_dir=dll_op_path ) 
        SConscript( 'libgeneric/AgediagnosticSConscript', variant_dir=dll_op_path ) 
        SConscript( 'libgeneric/Hivartstagingbycd4diagnosticSConscript', variant_dir=dll_op_path )
        SConscript( 'libgeneric/Hivartstagingcd4agnosticdiagnosticSConscript', variant_dir=dll_op_path ) 
        SConscript( 'libgeneric/HivdelayedinterventionSConscript', variant_dir=dll_op_path ) 
        SConscript( 'libgeneric/HivdrawbloodSConscript', variant_dir=dll_op_path ) 
        SConscript( 'libgeneric/HivpiecewisebyyearandsexdiagnosticSConscript', variant_dir=dll_op_path ) 
        SConscript( 'libgeneric/HivpreartnotificationSConscript', variant_dir=dll_op_path ) 
        SConscript( 'libgeneric/HivrandomchoiceSConscript', variant_dir=dll_op_path ) 
        SConscript( 'libgeneric/HivrapidhivdiagnosticSConscript', variant_dir=dll_op_path ) 
        SConscript( 'libgeneric/HivsigmoidbyyearandsexdiagnosticSConscript', variant_dir=dll_op_path ) 
        SConscript( 'libgeneric/HivsimplediagnosticSConscript', variant_dir=dll_op_path )
        SConscript( 'libgeneric/HivmuxerSConscript', variant_dir=dll_op_path )
        SConscript( 'libgeneric/PmtctSConscript', variant_dir=dll_op_path )

    if env['DiseaseDll'] == "PY":
        SConscript( 'libpy/PySConscript', variant_dir=dll_op_path )

    # Polio
    # NOT YET SConscript( 'libgeneric/PoliovaccineSConscript' )
    SConscript( 'libgeneric/BirthtriggeredSConscript', variant_dir=dll_op_path )
    SConscript( 'libgeneric/BroadcasteventSConscript', variant_dir=dll_op_path ) 
    SConscript( 'libgeneric/CalendarSConscript', variant_dir=dll_op_path )
    SConscript( 'libgeneric/DelayedInterventionSConscript', variant_dir=dll_op_path )
    SConscript( 'libgeneric/DiagnosticsSConscript', variant_dir=dll_op_path )
    SConscript( 'libgeneric/HealthseekingbehaviorSConscript', variant_dir=dll_op_path )
    SConscript( 'libgeneric/MultiinterventiondistributorSConscript', variant_dir=dll_op_path )
    SConscript( 'libgeneric/NodeLevelHealthtriggeredSConscript', variant_dir=dll_op_path )
    SConscript( 'libgeneric/OutbreakSConscript', variant_dir=dll_op_path )
    SConscript( 'libgeneric/OutbreakIndividualSConscript', variant_dir=dll_op_path )
    SConscript( 'libgeneric/PropertyvaluechangerSConscript', variant_dir=dll_op_path )
    SConscript( 'libgeneric/SimplevaccineSConscript', variant_dir=dll_op_path )

# Finally executable
SConscript('Eradication/SConscript')

def OptionalScript(sconscript_name):
    sconscript_path = os.path.join(Dir('#').abspath, sconscript_name)
    if os.path.isfile(sconscript_path):
        SConscript(sconscript_name)
    else:
        print("Skipping missing script: '{0}'".format(sconscript_path))

disease = "ALL"
if 'Disease' in env and len(env['Disease']) > 0:
    disease = env["Disease"]

if os.sys.platform == 'win32':
    if disease != "Typhoid":
        OptionalScript('reporters/SConscript_Generic_AgeAtInfection')
        OptionalScript('reporters/SConscript_Generic_AgeAtInfectionHistogram')
        OptionalScript('reporters/SConscript_Generic_Basic')
        OptionalScript('reporters/SConscript_Generic_EventCounter')
        OptionalScript('reporters/SConscript_Generic_HumanMigrationTracking')
        OptionalScript('reporters/SConscript_Generic_KmlDemo')
        OptionalScript('reporters/SConscript_Generic_NodeDemographics')

    if( (disease == "ALL") or (disease == "HIV") ):
        OptionalScript('reporters/SConscript_HIV_WHO2015')

    if( (disease == "ALL") or (disease == "Malaria") ):
        OptionalScript('reporters/SConscript_Malaria_Filtered')
        OptionalScript('reporters/SConscript_Malaria_Immunity')
        OptionalScript('reporters/SConscript_Malaria_Patient')
        OptionalScript('reporters/SConscript_Malaria_Summary')
        OptionalScript('reporters/SConscript_Malaria_Survey')

    if( (disease == "ALL") or (disease == "Polio") ):
        OptionalScript('reporters/SConscript_Polio_IndividualInfections')
        OptionalScript('reporters/SConscript_Polio_Survey')
        OptionalScript('reporters/SConscript_Polio_VirusPopulation')

    if( (disease == "ALL") or (disease == "TB") ):
        OptionalScript('reporters/SConscript_TB_Patient')
        OptionalScript('reporters/SConscript_TB_ReportScenarios')

    if( (disease == "ALL") or (disease == "STI") or (disease == "HIV") ):
        OptionalScript('reporters/SConscript_STI_RelationshipMigrationTracking')
        OptionalScript('reporters/SConscript_STI_RelationshipQueue')
        OptionalScript('reporters/SConscript_STI_RelationshipCensus')

    if( (disease == "ALL") or (disease == "Vector") or (disease == "Malaria") ):
        OptionalScript('reporters/SConscript_Vector_VectorHabitat')
        OptionalScript('reporters/SConscript_Vector_VectorMigration')
        OptionalScript('reporters/SConscript_Vector_VectorStats')

if( disease == "ALL"):
    OptionalScript('UnitTest++/SConscript')
    OptionalScript('componentTests/SConscript')
