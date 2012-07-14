#include "terrainmaterial.hpp"

#include <OgreTerrain.h>

#include <extern/shiny/Main/Factory.hpp>

namespace
{
    Ogre::String getComponent (int num)
    {
        if (num == 0)
            return "x";
        else if (num == 1)
            return "y";
        else if (num == 2)
            return "z";
        else
            return "w";
    }
}


namespace MWRender
{

    TerrainMaterial::TerrainMaterial()
    {
        mLayerDecl.samplers.push_back(Ogre::TerrainLayerSampler("albedo_specular", Ogre::PF_BYTE_RGBA));
        //mLayerDecl.samplers.push_back(Ogre::TerrainLayerSampler("normal_height", Ogre::PF_BYTE_RGBA));

        mLayerDecl.elements.push_back(
            Ogre::TerrainLayerSamplerElement(0, Ogre::TLSS_ALBEDO, 0, 3));
        //mLayerDecl.elements.push_back(
        //	Ogre::TerrainLayerSamplerElement(0, Ogre::TLSS_SPECULAR, 3, 1));
        //mLayerDecl.elements.push_back(
        //	Ogre::TerrainLayerSamplerElement(1, Ogre::TLSS_NORMAL, 0, 3));
        //mLayerDecl.elements.push_back(
        //	Ogre::TerrainLayerSamplerElement(1, Ogre::TLSS_HEIGHT, 3, 1));


        mProfiles.push_back(OGRE_NEW Profile(this, "SM2", "Profile for rendering on Shader Model 2 capable cards"));
        setActiveProfile("SM2");
    }

    // -----------------------------------------------------------------------------------------------------------------------

    TerrainMaterial::Profile::Profile(Ogre::TerrainMaterialGenerator* parent, const Ogre::String& name, const Ogre::String& desc)
        : Ogre::TerrainMaterialGenerator::Profile(parent, name, desc)
        , mGlobalColourMap(false)
    {
    }

    TerrainMaterial::Profile::~Profile()
    {
    }


    Ogre::MaterialPtr TerrainMaterial::Profile::generate(const Ogre::Terrain* terrain)
    {
        const Ogre::String& matName = terrain->getMaterialName();

        sh::Factory::getInstance().destroyMaterialInstance (matName);

        Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().getByName(matName);
        if (!mat.isNull())
            Ogre::MaterialManager::getSingleton().remove(matName);

        mMaterial = sh::Factory::getInstance().createMaterialInstance (matName);

        mMaterial->setProperty ("allow_fixed_function", sh::makeProperty<sh::BooleanValue>(new sh::BooleanValue(false)));

        createPass(0, terrain);

        return Ogre::MaterialManager::getSingleton().getByName(matName);
    }

    void TerrainMaterial::Profile::setGlobalColourMapEnabled (bool enabled)
    {
        mGlobalColourMap = enabled;
        mParent->_markChanged();
    }

    void TerrainMaterial::Profile::setGlobalColourMap (Ogre::Terrain* terrain, const std::string& name)
    {
        sh::Factory::getInstance ().setTextureAlias (terrain->getMaterialName () + "_colourMap", name);
    }

    int TerrainMaterial::Profile::getLayersPerPass () const
    {
        return 11;
    }

    void TerrainMaterial::Profile::createPass (int index, const Ogre::Terrain* terrain)
    {
        int layerOffset = index * getLayersPerPass();

        sh::MaterialInstancePass* p = mMaterial->createPass ();

        p->setProperty ("vertex_program", sh::makeProperty<sh::StringValue>(new sh::StringValue("terrain_vertex")));
        p->setProperty ("fragment_program", sh::makeProperty<sh::StringValue>(new sh::StringValue("terrain_fragment")));

        p->mShaderProperties.setProperty ("colour_map", sh::makeProperty<sh::BooleanValue>(new sh::BooleanValue(mGlobalColourMap)));

        // global colour map
        sh::MaterialInstanceTextureUnit* colourMap = p->createTextureUnit ("colourMap");
        colourMap->setProperty ("texture_alias", sh::makeProperty<sh::StringValue> (new sh::StringValue(mMaterial->getName() + "_colourMap")));
        colourMap->setProperty ("tex_address_mode", sh::makeProperty<sh::StringValue> (new sh::StringValue("clamp")));

        // global normal map
        sh::MaterialInstanceTextureUnit* normalMap = p->createTextureUnit ("normalMap");
        normalMap->setProperty ("direct_texture", sh::makeProperty<sh::StringValue> (new sh::StringValue(terrain->getTerrainNormalMap ()->getName())));
        normalMap->setProperty ("tex_address_mode", sh::makeProperty<sh::StringValue> (new sh::StringValue("clamp")));

        uint maxLayers = getMaxLayers(terrain);
        uint numBlendTextures = std::min(terrain->getBlendTextureCount(maxLayers), terrain->getBlendTextureCount());
        uint numLayers = std::min(maxLayers, static_cast<uint>(terrain->getLayerCount()));

        p->mShaderProperties.setProperty ("num_layers", sh::makeProperty<sh::StringValue>(new sh::StringValue(Ogre::StringConverter::toString(numLayers))));
        p->mShaderProperties.setProperty ("num_blendmaps", sh::makeProperty<sh::StringValue>(new sh::StringValue(Ogre::StringConverter::toString(numBlendTextures))));

        // blend maps
        for (uint i = 0; i < numBlendTextures; ++i)
        {
            sh::MaterialInstanceTextureUnit* blendTex = p->createTextureUnit ("blendMap" + Ogre::StringConverter::toString(i));
            blendTex->setProperty ("direct_texture", sh::makeProperty<sh::StringValue> (new sh::StringValue(terrain->getBlendTextureName(i))));
            blendTex->setProperty ("tex_address_mode", sh::makeProperty<sh::StringValue> (new sh::StringValue("clamp")));
        }

        // layer maps
        for (uint i = 0; i < numLayers; ++i)
        {
            sh::MaterialInstanceTextureUnit* diffuseTex = p->createTextureUnit ("diffuseMap" + Ogre::StringConverter::toString(i));
            diffuseTex->setProperty ("direct_texture", sh::makeProperty<sh::StringValue> (new sh::StringValue(terrain->getLayerTextureName(i, 0))));
            p->mShaderProperties.setProperty ("blendmap_index_" + Ogre::StringConverter::toString(i),
                sh::makeProperty<sh::StringValue>(new sh::StringValue(Ogre::StringConverter::toString(int((i-1) / 4)))));
            p->mShaderProperties.setProperty ("blendmap_component_" + Ogre::StringConverter::toString(i),
                sh::makeProperty<sh::StringValue>(new sh::StringValue(getComponent(int((i-1) % 4)))));
        }
    }

    Ogre::MaterialPtr TerrainMaterial::Profile::generateForCompositeMap(const Ogre::Terrain* terrain)
    {
        throw std::runtime_error ("composite map not supported");
    }

    Ogre::uint8 TerrainMaterial::Profile::getMaxLayers(const Ogre::Terrain* terrain) const
    {
        return 32;
    }

    void TerrainMaterial::Profile::updateParams(const Ogre::MaterialPtr& mat, const Ogre::Terrain* terrain)
    {
    }

    void TerrainMaterial::Profile::updateParamsForCompositeMap(const Ogre::MaterialPtr& mat, const Ogre::Terrain* terrain)
    {
    }

    void TerrainMaterial::Profile::requestOptions(Ogre::Terrain* terrain)
    {
        terrain->_setMorphRequired(true);
        terrain->_setNormalMapRequired(true); // global normal map
        terrain->_setLightMapRequired(false);
        terrain->_setCompositeMapRequired(false);
    }

}
