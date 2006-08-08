from btor import BtoRGUIClasses as ui
from btor import BtoRMain
from btor import BtoRAdapterClasses
reload(BtoRMain)
# backGround = [0.65, 0.65, 0.65]

instBtoREvtManager = ui.EventManager()  # initialize the event manager first
setattr(BtoRMain, "instBtoREvtManager", instBtoREvtManager)

instBtoRSettings = BtoRMain.BtoRSettings() # then bring up the settings
setattr(BtoRMain, "instBtoRSettings", instBtoRSettings)

instBtoRSceneSettings = BtoRMain.SceneSettings() # then scene settings
setattr(BtoRMain, "instBtoRSceneSettings", instBtoRSceneSettings)


setattr(BtoRMain, "instBtoRMaterials", BtoRMain.MaterialList())
instBtoRMaterials = getattr(BtoRMain, "instBtoRMaterials")

instBtoRGroupList = BtoRMain.GroupList()
setattr(BtoRMain, "instBtoRGroupList", instBtoRGroupList)

instBtoRHelp = BtoRMain.HelpWindow(25)
setattr(BtoRMain, "instBtoRHelp", instBtoRHelp)

instBtoRObjects = BtoRMain.ObjectEditor()
setattr(BtoRMain, "instBtoRObjects", instBtoRObjects)

instBtoRExport = BtoRMain.ExportUI()
setattr(BtoRMain, "instBtoRExport", instBtoRExport)

instBtoRMain = BtoRMain.MainUI()
setattr(BtoRMain, "instBtoRMain", instBtoRMain)


if instBtoRSettings.haveSetup == False:
	instBtoREvtManager.addElement(instBtoRSettings.getEditor())		
else:
	#matEditor = Material(None, None)
	instBtoREvtManager.addElement(instBtoRMain.getEditor())
	#evt_manager.addElement(matEditor.getEditor())

instBtoREvtManager.register() # fire it up