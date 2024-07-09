from matplotlib import pyplot as plt
import json
import pandas as pd
import dtk_test.dtk_sft as dtk_sft
import os

# Plot all channels using linear and scatter plots
def plot_all(df):
        from matplotlib import pyplot as plt
        from itertools import cycle
        channel_names = sorted(list(df))
       
        channels_data = df
        j = 1
        color = cycle(["blue","darkgreen", "darkmagenta",  "darkblue", "indigo", "orange"])
        fig = plt.figure().set_size_inches(15,10)
        plt.subplots_adjust(left=0.1,bottom=0.1,right=0.9, top=.92, wspace=0.4, hspace=0.8)
        plt.style.use('ggplot')
        plt.rc('xtick', labelsize=6)   
        plt.rc('ytick', labelsize=6)    
        for channel in channel_names:
            y = channels_data[channel]['Data']
            x = range(0, len(y))
            plt.subplot(7, 6, j)
            plt.plot( x, y, color = "grey" , linewidth=.3)
            plt.scatter(x, y, marker = 'o', color =next(color) , s = 1)
            plt.title(channel.replace(",", ",\n").replace(":",":\n"), fontsize=7)
            j+=1
        plt.savefig("All_Channels.png")
        if dtk_sft.check_for_plotting(): plt.show()
        plt.close(fig)
        
# Plot all Properties
def property_report(path="output", filename="PropertyReport.json"):
        propertyfile = os.path.join(path, filename )
        rawdata =""
        with open(propertyfile) as json_file:
            rawdata = json.load(json_file)

        pr_data = pd.DataFrame(rawdata['Channels'])
        pr_channel_names = pr_data.columns.sort_values()
        colors = ["blue","deepskyblue", "orchid", "chocolate", "blueviolet", "limegreen", "deeppink", "darkmagenta", "slateblue", "red", "green", "pink"]
        
        for i in range(0, 35, 12):
            sp = 1
            c = 0
            fig = plt.figure().set_size_inches(12, 10)
            plt.subplots_adjust(left=0.1,bottom=0.1,right=0.9, top=.92, wspace=0.4, hspace=0.8)
            # 12 channels:
            subset = pr_channel_names[i:i+12]
            
            for validate_channel in subset:
                y = pr_data[validate_channel]['Data']
                x = range(0, len(y))
                plt.subplot(4, 3, sp)
                plt.plot( x, y, color = "lightblue" , linewidth=.4)
                plt.scatter(x, y, marker = 'o', color =colors[c] , s = 3)
                plt.title(validate_channel.replace(",", "\n"), fontsize=7)
                sp = sp+1
                c = c+1
                plt.savefig(f"Property_Report_Set_{i}.png")

            if dtk_sft.check_for_plotting(): plt.show()
            plt.close(fig)