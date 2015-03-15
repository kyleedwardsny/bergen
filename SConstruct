# SConstruct
# Copyright (C) 2015 Kyle Edwards <kyleedwardsny@gmail.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

import os, subprocess

vars = Variables(".configuration.py")
vars.AddVariables(						\
	BoolVariable("TEST", "Unset to skip unit tests", True)	\
)

env = Environment(variables = vars)

Help(vars.GenerateHelpText(env))

cleaning = env.GetOption("clean")
help = env.GetOption("help")

if help:
	Return()

def configure_script(env, vars):
	conf = Configure(env)
	if not conf.CheckHeader("check.h") or not conf.CheckLib("check"):
		print("WARNING: check not found, skipping unit tests!")
		env.Replace(TEST = False)
	vars.Save(".configuration.py", env)
	return conf.Finish()

def unit_test_func(source, target, **kargs):
	for s in source:
		p = subprocess.Popen(s.abspath)
		p.wait()
		if p.returncode != 0:
			return p.returncode

	for t in target:
		f = open(t.abspath, "wa")
		f.close()

	return 0

unit_test = env.Builder(action = unit_test_func)
env.Append(BUILDERS = {"UnitTest": unit_test})

env.Append(CPPPATH = [Dir("include")])
env.Append(LIBPATH = [Dir("build/libbergen")])
env.Append(CFLAGS = ["-std=c99", "-Wall", "-pedantic"])

if not os.path.exists("config.log") and not cleaning:
	env = configure_script(env, vars)

distclean_files = [		\
	"config.log",		\
	".configuration.py",	\
	".sconf_temp",		\
	".sconsign.dblite",	\
	"build",		\
]

env.Export({"env": env})

env.SConscript("bergen/SConscript", variant_dir = "build/bergen", duplicate = 0)
env.SConscript("libbergen/SConscript", variant_dir = "build/libbergen", duplicate = 0)

if env["TEST"]:
	env.SConscript("test/SConscript", variant_dir = "build/test", duplicate = 0)

distclean = env.Clean("distclean", distclean_files)
env.Clean("clean", "build")
if cleaning:
	env.Default("clean")
