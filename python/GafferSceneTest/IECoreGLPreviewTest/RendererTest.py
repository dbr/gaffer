##########################################################################
#
#  Copyright (c) 2017, Image Engine Design Inc. All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are
#  met:
#
#      * Redistributions of source code must retain the above
#        copyright notice, this list of conditions and the following
#        disclaimer.
#
#      * Redistributions in binary form must reproduce the above
#        copyright notice, this list of conditions and the following
#        disclaimer in the documentation and/or other materials provided with
#        the distribution.
#
#      * Neither the name of John Haddon nor the names of
#        any other contributors to this software may be used to endorse or
#        promote products derived from this software without specific prior
#        written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
#  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
#  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
#  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
##########################################################################

import os
import unittest

import IECore
import IECoreGL

import GafferTest
import GafferScene

IECoreGL.init( False )

class RendererTest( GafferTest.TestCase ) :

	def testFactory( self ) :

		self.assertTrue( "OpenGL" in GafferScene.Private.IECoreScenePreview.Renderer.types() )

		r = GafferScene.Private.IECoreScenePreview.Renderer.create( "OpenGL" )
		self.assertTrue( isinstance( r, GafferScene.Private.IECoreScenePreview.Renderer ) )

	def testOtherRendererAttributes( self ) :

		# Attributes destined for other renderers should be silently ignored

		renderer = GafferScene.Private.IECoreScenePreview.Renderer.create( "OpenGL" )

		with IECore.CapturingMessageHandler() as handler :

			renderer.attributes(
				IECore.CompoundObject( {
					"ai:visibility:camera" : IECore.IntData( 0 )
				} )
			)

		self.assertEqual( len( handler.messages ), 0 )

	def testPrimVars( self ) :

		renderer = GafferScene.Private.IECoreScenePreview.Renderer.create( "OpenGL" )
		renderer.output( "test", IECore.Display( self.temporaryDirectory() + "/testPrimVars.tif", "tiff", "rgba", {} ) )

		fragmentSource = """
		uniform float red;
		uniform float green;
		uniform float blue;

		void main()
		{
			gl_FragColor = vec4( red, green, blue, 1 );
		}
		"""

		attributes = renderer.attributes(
			IECore.CompoundObject( {
				"gl:surface" : IECore.ObjectVector( [
					IECore.Shader( "rgbColor", "surface", { "gl:fragmentSource" : fragmentSource } )
				] )
			} )
		)

		def sphere( red, green, blue ) :

			s = IECore.SpherePrimitive()
			s["red"] = IECore.PrimitiveVariable( IECore.PrimitiveVariable.Interpolation.Constant, IECore.FloatData( red ) )
			s["green"] = IECore.PrimitiveVariable( IECore.PrimitiveVariable.Interpolation.Constant, IECore.FloatData( green ) )
			s["blue"] = IECore.PrimitiveVariable( IECore.PrimitiveVariable.Interpolation.Constant, IECore.FloatData( blue ) )

			return s

		renderer.object(
			"redSphere",
			sphere( 1, 0, 0 ),
			attributes
		).transform(
			IECore.M44f().translate( IECore.V3f( 0, 0, -5 ) )
		)

		renderer.object(
			"greenSphere",
			sphere( 0, 1, 0 ),
			attributes
		).transform(
			IECore.M44f().translate( IECore.V3f( -1, 0, -5 ) )
		)

		renderer.object(
			"blueSphere",
			sphere( 0, 0, 1 ),
			attributes
		).transform(
			IECore.M44f().translate( IECore.V3f( 1, 0, -5 ) )
		)

		renderer.render()

		image = IECore.Reader.create(  self.temporaryDirectory() + "/testPrimVars.tif" ).read()
		dimensions = image.dataWindow.size() + IECore.V2i( 1 )
		index = dimensions.x * int( dimensions.y * 0.5 )
		self.assertEqual( image["R"][index], 0 )
		self.assertEqual( image["G"][index], 1 )
		self.assertEqual( image["B"][index], 0 )

		index = dimensions.x * int(dimensions.y * 0.5) + int( dimensions.x * 0.5 )
		self.assertEqual( image["R"][index], 1 )
		self.assertEqual( image["G"][index], 0 )
		self.assertEqual( image["B"][index], 0 )

		index = dimensions.x * int(dimensions.y * 0.5) + int( dimensions.x * 1 ) - 1
		self.assertEqual( image["R"][index], 0 )
		self.assertEqual( image["G"][index], 0 )
		self.assertEqual( image["B"][index], 1 )

	def testShaderParameters( self ) :

		renderer = GafferScene.Private.IECoreScenePreview.Renderer.create( "OpenGL" )
		renderer.output( "test", IECore.Display( self.temporaryDirectory() + "/testShaderParameters.tif", "tiff", "rgba", {} ) )

		fragmentSource = """
		uniform vec3 colorValue;
		void main()
		{
			gl_FragColor = vec4( colorValue, 1 );
		}
		"""

		attributes = renderer.attributes(
			IECore.CompoundObject( {
				"gl:surface" : IECore.ObjectVector( [
					IECore.Shader(
						"color",
						"surface",
						{
							"gl:fragmentSource" : fragmentSource,
							"colorValue" : IECore.Color3f( 1, 0, 0 )
						}
					)
				] )
			} )
		)

		renderer.object(
			"sphere",
			IECore.SpherePrimitive(),
			attributes
		).transform(
			IECore.M44f().translate( IECore.V3f( 0, 0, -5 ) )
		)

		renderer.render()

if __name__ == "__main__":
	unittest.main()
