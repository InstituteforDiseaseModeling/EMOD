# Data for evaluations


coital_act_report_expected_columns=    ["A_PartnersWith_IP='IsPurifier:YES'",
                                    "A_PartnersWith_IP='CanInheritWives:YES'",
                                    "A_PartnersWith_IP='WifeInheritanceStage:PURIFICATION'",
                                    "A_PartnersWith_IP='WifeInheritanceStage:INHERITED_MARRIAGE'",
                                    "B_PartnersWith_IP='IsPurifier:YES'",
                                    "B_PartnersWith_IP='CanInheritWives:YES'",
                                    "B_PartnersWith_IP='WifeInheritanceStage:PURIFICATION'",
                                    "B_PartnersWith_IP='WifeInheritanceStage:INHERITED_MARRIAGE'"]


inset_chart_channel_sets = {             
                            "s1": { 
                                        "desc": "Relationships Plots",
                                        "channels" : [   
                                            'Active COMMERCIAL Relationships', 
                                            'Active INFORMAL Relationships',
                                            'Active MARITAL Relationships', 
                                            'Active TRANSITORY Relationships',
                                            'Num Rels Outside PFA', 
                                            'Number of Individuals Ever in a Relationship', 
                                            'Paired People']
                                            },
                            "s2": {
                                        "desc": "Wife Inheritance Related Channels Group",
                                        "channels": [   'Has_WaitToStartPurifyingRelationship', 
                                                        'HasIP_WifeInheritanceStage:PURIFICATION',
                                                        'HasIP_WifeInheritanceStage:INHERITED_MARRIAGE',
                                            ]
                                        },
                            "s3": {
                                        "desc": "Pregnancy Channels",
                                        "channels":[    'New Pregnancies', 
                                                        'Currently Pregnant',
                                                        'Possible Mothers'],
                                    },
                            "s4" : {
                                        "desc" : "Wife Inheritance and Diseases Correlation",
                                        "channels" : [ 'HasIP_WifeInheritanceStage:INHERITED_MARRIAGE',
                                                        'Disease Deaths', 
                                                        'Infectious Population', 
                                                        'Newly Symptomatic']
                                    },
                            "s5": {
                                        "desc" : "Infections from Non PFA relationships",
                                            "channels" : ["Fraction of New Infections From Rels Outside PFA"]
                                            },
                            "s6": {
                                        "desc": "From NA to Inherited_Marriage",
                                        "channels": [   'HasIP_WifeInheritanceStage:NA',       
                                                        'HasIP_WifeInheritanceStage:INHERITED_MARRIAGE']
                                            }
                            }
