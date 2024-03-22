import hilog from '@ohos.hilog';
import testNapi from 'libentry.so'
import worker from '@ohos.worker';
import StyleConstant from '../common/constant/StyleConstant';
import taskpool from '@ohos.taskpool';

@Concurrent
function httpGet(url: string) : string {
  const body: string = testNapi.HttpGet(url);
  return body;
}

@Concurrent
function Add(num1, num2) : number {
  return testNapi.Add(num1, num2);
}

async function asyncTask() : Promise<void> {
  try {
    let task = new taskpool.Task(Add, 10, 100);
    let num = await taskpool.execute(task);
    console.info('------------------------------>Add Result: ', num);
  } catch (e) {
    console.error("----------------------------->Taskpool execute Add error: " + e);
  }

  try {
    let url: string = "http://www.baidu.com/";
    let task = new taskpool.Task(httpGet, url);
    let body = await taskpool.execute(task);
    if (body != null) {
      //console.info('----------------------------->body:' + String(body));
      console.info('----------------------------->body ok!');
    } else {
      console.info("------------------------------>body null");
    }
  } catch (e) {
    console.error("----------------------------->Taskpool execute HttpGet error: " + e);
  }
}

@Entry
@Component
struct Index {
  @State message: string = 'Hello World'

  build() {
    Row() {
      Column() {
        Text(this.message)
          .fontSize(50)
          .fontWeight(FontWeight.Bold)
          .onClick(() => {
            asyncTask();
            //hilog.info(0x0000, 'testTag', 'Test NAPI 2 + 3 = %{public}d', testNapi.Add(20, 30));
          })
      }
      .width('100%')
    }
    .height('100%')
  }
}
