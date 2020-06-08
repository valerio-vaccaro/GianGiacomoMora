import requests
from zipfile37 import ZipFile
import data_pb2

r = requests.get('https://get.immuni.gov.it/v1/keys/index').json()
print(r)

for i in range(r['oldest'], r['newest']+1):
    print(str(i)+'.zip')
    r = requests.get('https://get.immuni.gov.it/v1/keys/'+str(i))
    open('./dataset/'+str(i)+'.zip', 'wb').write(r.content)
    with ZipFile('./dataset/'+str(i)+'.zip', 'r') as zip:
        zip.printdir()
        print('Extracting all the files now...')
        zip.extractall(path='./dataset/'+str(i))
        print('Done!')

        with open('./dataset/'+str(i)+'/export.bin', 'rb') as f:
            decoded = data_pb2.TemporaryExposureKeyExport()
            decoded.ParseFromString(f.read()[16:])
            # do something with read_metric
            print(decoded)

        with open('./dataset/'+str(i)+'/export.sig', 'rb') as f:
            decoded = data_pb2.TEKSignatureList()
            decoded.ParseFromString(f.read())
            # do something with read_metric
            print(decoded)
