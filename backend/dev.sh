# check argument --new is passed
if [ "$1" == "--new" ]; then
    # print creating new installation
    echo "INFO: Creating new installation (--new flag dectected)"

    # remove existing virtual environment
    rm -rf venv

    # setup python virtual environment
    python3 -m venv venv
    # activate virtual environment
    source venv/bin/activate

    # install dependencies
    pip install -r requirements.txt
fi

# activate virtual environment
source venv/bin/activate

# run fastapi using uvicorn
uvicorn main:app --reload
