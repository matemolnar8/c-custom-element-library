import { HelloWasmComponent } from "./hello-wasm-component";
import { assertAndGet } from "./util/assert-value";

class HelloWorldElement extends HTMLElement {
  helloWasmComponent: HelloWasmComponent | undefined;

  constructor() {
    super();
    this.attachShadow({ mode: "open" });

    this.shadowRoot.innerHTML = `
      <style>
        :host {
          display: block;
          width: 100%;
          height: 100%;
          background-color: white;
        }
      </style>
      <div id="hello"></div>
    `;
  }

  get shadowRoot() {
    return assertAndGet(super.shadowRoot, "Could not find shadow root");
  }

  async connectedCallback() {
    this.helloWasmComponent = new HelloWasmComponent("./hello.wasm");
    await this.helloWasmComponent.init();

    const hello = this.shadowRoot.getElementById("hello");
    if (hello) {
      const render = () => {
        const result = this.helloWasmComponent!.render();
        const element = document.createElement(result.tag);
        element.textContent = result.text;
        hello.appendChild(element);
      };
      render();
    }
  }
}

customElements.define("hello-world", HelloWorldElement);
