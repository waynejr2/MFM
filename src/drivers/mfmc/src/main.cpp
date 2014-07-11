#include "main.h"

#ifdef MFM_GUI_DRIVER
  #include "AbstractSliderConfig.h"
#endif

namespace MFM
{
  typedef ParamConfig<64,4,8,40> OurParamConfig;
  typedef P3Atom<OurParamConfig> OurAtom;
  typedef CoreConfig<OurAtom, OurParamConfig> OurCoreConfig;
  typedef GridConfig<OurCoreConfig, 5, 3> OurGridConfig;

#ifdef MFM_GUI_DRIVER
  typedef AbstractSliderConfig<OurCoreConfig> OurSliderConfig;
#endif

  struct MFMCDriver : public AbstractDualDriver<OurGridConfig>
  {
  private: typedef AbstractDualDriver<OurGridConfig> Super;

  public:
    virtual void AddDriverArguments()
    {
      Super::AddDriverArguments();
    }

    virtual void OnceOnly(VArguments& args)
    {

#ifdef MFM_GUI_DRIVER
      AbstractGUIDriver::m_startPaused = true;
#endif

      Super::OnceOnly(args);
    }

    virtual void ReinitPhysics()
    {
      OurGrid& m_grid = GetGrid();

      m_grid.Needed(Element_Dreg<OurCoreConfig>::THE_INSTANCE);
      m_grid.Needed(Element_Res<OurCoreConfig>::THE_INSTANCE);
      m_grid.Needed(Element_Wall<OurCoreConfig>::THE_INSTANCE);

#ifdef MFM_GUI_DRIVER

      Element<CC>* elem = &Element_Dreg<OurCoreConfig>::THE_INSTANCE;
      m_dregSliderConfig.SetElement(elem);

      /* Register sliders */
      AbstractGUIDriver::RegisterSliderConfig(&m_dregSliderConfig);
#endif
    }

#ifdef MFM_GUI_DRIVER

    class DregSliderConfig : public OurSliderConfig
    {
    public:
      virtual u32 SetupSliders(Slider* sliders, u32 maxSliders) const
      {
	sliders[0].SetText("RES Spawn Odds");
	sliders[0].SetMinValue(1);
	sliders[0].SetMaxValue(1000);
	sliders[0].SetExternalValue(Element_Dreg<CC>::THE_INSTANCE.GetResOddsPtr());
	sliders[0].SetValue(Element_Dreg<CC>::THE_INSTANCE.GetResOdds());

	sliders[1].SetText("DREG Spawn Odds");
	sliders[1].SetMinValue(1);
	sliders[1].SetMaxValue(1000);
	sliders[1].SetExternalValue(Element_Dreg<CC>::THE_INSTANCE.GetDregCreateOddsPtr());
	sliders[1].SetValue(Element_Dreg<CC>::THE_INSTANCE.GetDregCreateOdds());

	return 2;
      }
    }m_dregSliderConfig;

#endif

    virtual void ReinitEden()
    {

#ifdef MFM_GUI_DRIVER
      /* Register painting tools if we need them */
      AbstractGUIDriver::RegisterToolboxElement(&Element_Empty<OurCoreConfig>::THE_INSTANCE);
      AbstractGUIDriver::RegisterToolboxElement(&Element_Wall<OurCoreConfig>::THE_INSTANCE);
      AbstractGUIDriver::RegisterToolboxElement(&Element_Dreg<OurCoreConfig>::THE_INSTANCE);
      AbstractGUIDriver::RegisterToolboxElement(&Element_Res<OurCoreConfig>::THE_INSTANCE);
#endif
    }
  };
}

int main(int argc, const char** argv)
{
  MFM::LOG.SetByteSink(MFM::STDERR);
  MFM::LOG.SetLevel(MFM::LOG.ALL);

  MFM::MFMCDriver sim;
  sim.Init(argc, argv);

  sim.Reinit();

  sim.Run();

  return 0;
}
