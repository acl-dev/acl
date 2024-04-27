//import hilog from '@ohos.hilog';
//import worker from '@ohos.worker';
//import StyleConstant from '../common/constant/StyleConstant';
import testNapi from 'libentry.so'
import taskpool from '@ohos.taskpool';

@Concurrent
function httpGet(url: string) : string {
  const res: string = testNapi.HttpGet(url);
  return res
}

@Concurrent
function Add(num1, num2) : number {
  return testNapi.Add(num1, num2);
}

async function asyncTask() : Promise<string> {
  try {
    let task = new taskpool.Task(Add, 10, 100);
    let num = await taskpool.execute(task);
    console.info('-->Add Result: ', num);
  } catch (e) {
    console.error("-->Taskpool execute Add error: " + e);
  }

  try {
    let url: string = "http://www.baidu.com/";
    let task = new taskpool.Task(httpGet, url);
    let res = await taskpool.execute(task);
    if (res != null) {
      return String(res);
    } else {
      console.info("-->res null");
      return "";
    }
  } catch (e) {
    console.error("-->Taskpool execute HttpGet error: " + e);
    return "";
  }
}

@Entry
@Component
struct Index {
  @State message: string = 'Hello World'
  @State result: string = ""
  url: string = "http://www.baidu.com/"

  build() {
    Row() {
      Column() {

        Row() {
          Text("URL: ")
            .fontSize(20)
            .backgroundColor('#ff23d0c2')
            .fontWeight(FontWeight.Bold)
            .margin({ left: 10, right: 5 });

          TextInput({ placeholder: this.url })
            .backgroundColor(Color.White)
            .fontSize(20)
            .width('65%')
            .border({ width: 1 })
            .onChange((value: string) => {
              this.url = value;
            })
        }
        .width('100%')
        .margin( { bottom: 20 });

        Row() {
          Text(this.result)
            .width('90%')
            .border({ width: 1 })
            .fontSize(15)
            .baselineOffset(0)
            .padding(10)
            .height(450)
            .margin({ bottom: 50 })
            .maxLines(50)
            .copyOption(CopyOptions.LocalDevice)
        }

        Row() {
          Button('下载')
            .width('200vp')
            .height('60vp')
            .fontSize('16fp')
            .fontWeight(FontWeight.Bold)
            .borderRadius('20vp')
            .margin('12vp')
            .onClick(() => {
              asyncTask().then((res) => {
                this.result = res;
              });
            })
        }

        Row() {
          Button('清空')
            .width('200vp')
            .height('60vp')
            .fontSize('16fp')
            .fontWeight(FontWeight.Bold)
            .borderRadius('20vp')
            .margin('12vp')
            .onClick(() => {
              this.result = "";
            })
        }

      } // Column()
      .width('100%')
      .margin( { bottom: 50 });

    } // Row()
    .height('100%')
  } // build

} // Index
