#!/usr/bin/python
from sqlalchemy import create_engine, MetaData, Table, and_
from sqlalchemy.orm import mapper, sessionmaker
import sys
import pdb


class Config(object):
    pass

class Demographics(object):
    pass

class CampaignClasses(object):
    pass

class CampaignParams(object):
    pass

def loadSession():
    engine = create_engine(
        "mysql://braybaud:1!Zu5=7!@10.210.40.49/paramdb",
        isolation_level="READ UNCOMMITTED",
        echo=False
    )

    metadata = MetaData(engine)
    config_table = Table('config_param', metadata, autoload=True)
    mapper(Config, config_table)

    demographics_table = Table('demographics_param', metadata, autoload=True)
    mapper(Demographics, demographics_table)

    campaign_params_table = Table('campaign_class', metadata, autoload=True)
    mapper(CampaignClasses, campaign_params_table)

    campaign_classes_table = Table('campaign_class_param', metadata, autoload=True)
    mapper(CampaignParams, campaign_classes_table)

    Session = sessionmaker(bind=engine)
    session = Session()
    return session

# There are some params that are duplicates. Ultimately these need to be solved 
# with uniqueness changes in db, but for now we blacklist.
iv_param_blacklist = [
        "Coverage_DESC_TEXT",
        "Start_Year_DESC_TEXT",
        "TB_Drug_Cure_Rate_DESC_TEXT",
        "TB_Drug_Inactivation_Rate_DESC_TEXT",
        "TB_Drug_Mortality_Rate_DESC_TEXT",
        "TB_Drug_Relapse_Rate_DESC_TEXT",
        "TB_Drug_Resistance_Rate_DESC_TEXT"
    ]

def dumpConfigDescriptions( session ): 
    print( "// This is a generated file. Please do not edit manually." )
    results = session.query(Config) 
    for param in results:
        #print("{p.Parameter} - {p.Type} ({p.Min} to {p.Max}): {p.DefaultValue}".format(p=parameter))
        param_name = param.Parameter
        #if param.Parameter in config_param_map:
            #param_name = config_param_map[ param_name ]
        print( '#define {0}_DESC_TEXT "{1}"'.format( param_name, param.ShortDesc ) )

def dumpDemogDescriptions( session ): 
    results = session.query(Demographics) 
    for param in results:
        print( '#define {0}_DESC_TEXT "{1}"'.format( param.Parameter, param.ShortDesc ) )
    print( '#include "missing_config_params.rc"' )

def dumpCampaignDescriptions( session ):
    results = session.query(CampaignParams) 
    for param in results:
        print( '#define {0}_DESC_TEXT "{1}"'.format( param.Parameter, param.ShortDesc ) )

def dumpCampaign2Descriptions( session ):
    results = session.query(CampaignClasses) 
    for param in results:
        print( '#define {0}_DESC_TEXT "{1}"'.format( param.Parameter, param.ShortDesc ) )

lines = []
def createAbbrevFromClassName( className ):
    abbrev = ""
    for letter in className:
        if letter.isupper():
            abbrev += letter
    return abbrev

def printCampaignParam( className, paramIn, shortDesc ):
    # convert className to abbreviation
    abbrev = ""
    for letter in className:
        if letter.isupper():
            abbrev += letter
    results = session.query(CampaignParams) 
        
    if paramIn == param.keyString or ( abbrev + "_" + paramIn ) == param.keyString:
        line = '#define {0} "{1}"'.format( paramIn + "_DESC_TEXT", shortDesc )
        if line not in lines:
            print( line )
            lines.append( line )
        return
    #print( "FAIL: " + paramIn )


def dumpCampaignParams( session ):
    print( "// This is a generated file. Please do not edit manually." )
    results_classes = session.query(CampaignClasses) 
    results_params = session.query(CampaignParams) 
    # Note to self: Don't write code like this. Unless you're using an unnecessary ORM instead of just doing queries...
    for iv in results_classes:
        # Join CampaignClass.classId wit CampaignParam.id
        print( "// " + str( iv.Class ) )
        for params in results_params:
            if iv.id == params.classId:
                #abbrev = createAbbrevFromClassName( iv.Class )
                ks = params.keyString
                if ks in iv_param_blacklist:
                    continue
                if ks.endswith( "_DESC" ):
                    #pdb.set_trace()
                    ks += "_TEXT"
                if ks.endswith( "_DESC_TEXT" ) == False:
                    ks += "_DESC_TEXT"
                #elif  "_DESC_DESC" in ks:
                    #ks = ks.replace( "DESC_DESC_", "DESC_" )
                line = '#define {0} "{1}"'.format( ks, params.ShortDesc )
                if line not in lines:
                    # print( "# abbrev = " + abbrev )
                    print( line )
                    lines.append( line )
                #printCampaignParam( params.Class, iv.parameter, iv.ShortDesc )
    print( '#include "missing_iv_params.rc"' )

if __name__ == "__main__":
    session = loadSession()
    # For more filtering capabilities please refer to:
    # http://www.leeladharan.com/sqlalchemy-query-with-or-and-like-common-filters
    #results = session.query(Config).filter(and_(Config.Category1 == "Immunity",
                                                #Config.SimType == "Generic"))

    if len( sys.argv ) > 1:
        if sys.argv[1] == "--iv":
            dumpCampaignParams( session )
        elif sys.argv[1] == "--config":
            dumpConfigDescriptions( session )
            dumpDemogDescriptions( session )
        elif sys.argv[1] == "--help":
            print( "Usage: get_params.py [--config|--iv]" )
    else:
        dumpConfigDescriptions( session )
        dumpDemogDescriptions( session )
