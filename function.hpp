class Function {
public:
    Function(char *name, int *argTypes);
    bool operator==(const Function &rhs);
    char * getName();
    int * getArgTypes();

    bool operator <(const Function& rhs) const
    {
        return name < rhs.name;
    }
private:
    char *name;
    int *argTypes;
};
