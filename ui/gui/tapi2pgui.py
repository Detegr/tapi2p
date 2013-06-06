import sys
from PyQt4 import QtGui

class Tapi2pMainWindow(QtGui.QWidget):
	def __init__(self, title):
		self.__mTabs=QtGui.QTabWidget()
		self.__mStatusWindow=Tapi2pStatusWindow()
		self.__mInputField=Tapi2pInputField()
		self.__mPeerList=Tapi2pPeerList()
		self.__mVBox=QtGui.QVBoxLayout()
		self.__mTabs.addTab(self.__mStatusWindow, "Main")

		super(Tapi2pMainWindow, self).__init__()
		self.mInit(title)

	def mInit(self, title):
		self.resize(800, 600)
		self.setWindowTitle(title)
		self.mAttachChildren()
		self.__mInputField.setFocus()
		self.show()

	def mAttachChildren(self):
		self.__mVBox.addWidget(self.__mTabs)
		self.__mVBox.addWidget(self.__mInputField)
		self.setLayout(self.__mVBox)
		pass

class Tapi2pStatusWindow(QtGui.QTextEdit):
	def __init__(self):
		super(Tapi2pStatusWindow, self).__init__()
		self.mInit()
	def mInit(self):
		self.setReadOnly(True)
		pass

class Tapi2pInputField(QtGui.QLineEdit):
	def __init__(self):
		super(Tapi2pInputField, self).__init__()
		self.mInit()
	def mInit(self):
		pass

class Tapi2pPeerList(QtGui.QListView):
	def __init__(self):
		super(Tapi2pPeerList, self).__init__()
		self.mInit()
	def mInit(self):
		pass

def main():
	app = QtGui.QApplication(sys.argv)
	tapi2pmain = Tapi2pMainWindow("tapi2p gui")
	sys.exit(app.exec_())

if __name__ == "__main__":
	main()
