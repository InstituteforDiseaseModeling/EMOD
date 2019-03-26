import regression_utils as ru

def application( config_file ):
    ru.flattenConfig( config_file )
    return "config.json"
