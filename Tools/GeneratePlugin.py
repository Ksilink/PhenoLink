# -*- coding: utf-8 -*-
"""
Created on Mon Aug 24 09:17:49 2020

@author: NicolasWiest-Daesslé
"""
# %%
# Load an output CSV file to Check the Matrix conformance


from string import Template
import os
import subprocess

infos = {
    "plugin_file": "Plugin Filename (path in the Process subdirectory like (Foo/MyPlugin => Foo/MyPlugin.cpp)",
    "plugin_name": "Plugin Name:",
    "plugin_path": "Plugin Path:"
}

for k, v in infos.items():
    infos[k] = input(infos[k])
# %%
# Some cleaning
infos["plugin_name"] = infos["plugin_name"].replace(" ", "_")
infos["UpperPlugin"] = infos["plugin_name"].upper()
infos["code_file"] = infos["plugin_file"]+".cpp"

# %%
os.makedirs(
    f'../../CheckoutPlugins/Process/{infos["plugin_file"]}/', exist_ok=True)
with open('Templates/h_template.tpl', 'r') as f:
    src = Template(f.read())
    header = src.substitute(infos)

    with open(f'../../CheckoutPlugins/Process/{infos["plugin_file"]}/{infos["plugin_name"]}.h', "w") as h:
        h.write(header)
#    print(header)

with open('Templates/cpp_template.tpl', 'r') as f:
    src = Template(f.read())
    cpp = src.substitute(infos)
    with open(f'../../CheckoutPlugins/Process/{infos["plugin_file"]}/{infos["plugin_name"]}.cpp', "w") as h:
        h.write(cpp)
#    print(cpp)

# %%

proj = "#"*84 + "\n"
proj += "#"*4 + ' ' + infos["plugin_name"] + \
    ' ' + '#'*(77-len(infos["plugin_name"]))+'\n'
proj += f'add_plugin({infos["plugin_name"]} \n    {infos["plugin_file"]}/{infos["plugin_name"]}.cpp {infos["plugin_file"]}/{infos["plugin_name"]}.h \n'
proj += "    LIBS\n    PluginCore)\n\n"
with open('../../CheckoutPlugins/Process/CMakeLists.txt', 'a') as cmake:
    cmake.write(proj)


# %%

subprocess.run(["git",
                "add",
                "CMakeLists.txt"
                f'{infos["plugin_file"]}/{infos["plugin_name"]}.cpp',
                f'{infos["plugin_file"]}/{infos["plugin_name"]}.h'],
               cwd="../../CheckoutPlugins/Process")


subprocess.run(["git",
                "commit", "-m", "Generate Plugin adding file"],
               cwd="../../CheckoutPlugins/Process")
