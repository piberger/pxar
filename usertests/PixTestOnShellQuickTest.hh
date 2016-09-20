#ifndef PIXTESTONSHELLQUICKTEST_H
#define PIXTESTONSHELLQUICKTEST_H

#include "PixTest.hh"

class DLLEXPORT PixTestOnShellQuickTest: public PixTest {

public:
    PixTestOnShellQuickTest(PixSetup *, std::string);
    PixTestOnShellQuickTest();
    virtual ~PixTestOnShellQuickTest();
    virtual bool setParameter(std::string parName, std::string sval);
    void runCommand(std::string);
    void init();
    void setToolTips();
    void bookHist(std::string);

    void doTest();

    void bbQuickTest();
    void hvQuickTest();
    std::vector<int> readBBvthrcomp();
    std::vector<int> readDbVana();
    std::vector<int> readDbCaldel();
    std::vector<int> readParametersFile(std::string fileName);

private:

    int fParNtrig;
    int fBBVthrcomp;
    int fParVcalS;

    ClassDef(PixTestOnShellQuickTest, 1)

};
#endif
